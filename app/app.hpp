#pragma once

#include "../third/crow_all.h"
#include "../third/json.hpp"
#include "md5.hpp"
#include "mockdb/easydb.hpp"

#include <sys/stat.h>
#include <fstream>
#include <map>
#include <random>
#include <sstream>
#include <string>

using json = nlohmann::json;

class DigestAuth {
public:
    DigestAuth(std::string realm, std::string user, std::string pass)
        : realm_(std::move(realm)), user_(std::move(user)), pass_(std::move(pass)) {}

    const std::string& realm() const { return realm_; }
    const std::string& user() const  { return user_; }

    std::string challenge() const {
        return "Digest realm=\"" + realm_
             + "\", nonce=\"" + generateNonce()
             + "\", qop=\"auth\"";
    }

    bool verify(const crow::request& req, const std::string& method) const {
        const std::string& auth = req.get_header_value("Authorization");
        if (auth.size() < 7 || auth.compare(0, 7, "Digest ") != 0) {
            return false;
        }

        std::map<std::string, std::string> p;
        parseHeader(auth.substr(7), p);

        auto get = [&](const char* k) -> std::string {
            auto it = p.find(k);
            return it == p.end() ? std::string() : it->second;
        };

        if (get("username") != user_) return false;
        if (get("realm")    != realm_) return false;
        const std::string nonce    = get("nonce");
        const std::string response = get("response");
        const std::string uri      = get("uri").empty() ? req.url : get("uri");
        const std::string qop      = get("qop");
        if (nonce.empty() || response.empty()) return false;

        const std::string ha1 = md5::hash(user_ + ":" + realm_ + ":" + pass_);
        const std::string ha2 = md5::hash(method + ":" + uri);

        std::string expected;
        if (qop == "auth") {
            const std::string nc    = get("nc");
            const std::string cnonce = get("cnonce");
            if (nc.empty() || cnonce.empty()) return false;
            expected = md5::hash(ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + ha2);
        } else {
            expected = md5::hash(ha1 + ":" + nonce + ":" + ha2);
        }
        return expected == response;
    }

private:
    static void parseHeader(const std::string& s, std::map<std::string, std::string>& p) {
        size_t pos = 0;
        while (pos < s.size()) {
            while (pos < s.size() && (s[pos] == ' ' || s[pos] == ',')) ++pos;
            size_t eq = s.find('=', pos);
            if (eq == std::string::npos) break;
            std::string key = s.substr(pos, eq - pos);
            size_t qs = eq + 1;
            if (qs < s.size() && s[qs] == '"') {
                ++qs;
                size_t qe = s.find('"', qs);
                if (qe == std::string::npos) break;
                p[key] = s.substr(qs, qe - qs);
                pos = qe + 1;
            } else {
                size_t qe = qs;
                while (qe < s.size() && s[qe] != ',') ++qe;
                while (qe > qs && s[qe - 1] == ' ') --qe;
                p[key] = s.substr(qs, qe - qs);
                pos = qe;
            }
        }
    }

    static std::string generateNonce() {
        static std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<> dist(0, 15);
        std::stringstream ss;
        for (int i = 0; i < 32; ++i) {
            ss << std::hex << dist(rng);
        }
        return ss.str();
    }

    std::string realm_;
    std::string user_;
    std::string pass_;
};

struct DigestAuthMiddleware {
    struct context {
        bool authorized = false;
    };

    DigestAuthMiddleware()
        : auth_("SignalController", "admin", "admin123") {}

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        if (req.url.compare(0, 5, "/api/") != 0) {
            return;
        }
        if (auth_.verify(req, crow::method_name(req.method))) {
            ctx.authorized = true;
            return;
        }
        res.code = 401;
        res.set_header("Content-Type", "application/json");
        res.write(json{
            {"error", "unauthorized"},
            {"challenge", auth_.challenge()}
        }.dump());
        res.end();
    }

    void after_handle(crow::request&, crow::response&, context&) {}

    const DigestAuth& auth() const { return auth_; }

private:
    DigestAuth auth_;
};

class RestApp {
public:
    RestApp() {
        app_.loglevel(crow::LogLevel::Warning);
        registerRoutes();
    }

    RestApp(const RestApp&) = delete;
    RestApp& operator=(const RestApp&) = delete;

    void run(uint16_t port = 8080) {
        app_.port(port).multithreaded().run();
    }

private:
    using AppT = crow::App<DigestAuthMiddleware>;
    AppT app_;
	EasyDB db;
    const std::string kWebDist_     = "../web/dist";

    static crow::response JsonResponse(int code, const json& body) {
        crow::response res(code, body.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
	template <typename T>
    crow::response renderPOST(const crow::request &req)
    {
        T t;
        json j = json::parse(req.body, nullptr, false);
        if (!j.is_discarded()) {
            j.get_to(t);
        }
        db.write(t);
        return std::move(crow::response(200,"OK"));
    }
    template <typename T>
    crow::response renderGET(const crow::request &req)
    {
        T t;
        db.read(t);
        return crow::response(200, json(t).dump());
    }

    void registerRoutes() {
        CROW_ROUTE(app_, "/api/Auth/Check")
            .methods(crow::HTTPMethod::Get)
        ([this](const crow::request&) {
            const auto& mw = app_.get_middleware<DigestAuthMiddleware>();
            return JsonResponse(200, json{{"user", mw.auth().user()}});
        });

        CROW_ROUTE(app_, "/api/Config/DeviceName")
            .methods(crow::HTTPMethod::Get)
        ([this](const crow::request& req) {
            return renderGET<DeviceName>(req);
        });

        CROW_ROUTE(app_, "/api/Config/DeviceName")
            .methods(crow::HTTPMethod::Post)
        ([this](const crow::request& req) {
            return renderPOST<DeviceName>(req);
        });

        CROW_ROUTE(app_, "/")
        ([this] {
            crow::response res;
            res.set_static_file_info_unsafe(kWebDist_ + "/index.html");
            return res;
        });

        CROW_ROUTE(app_, "/static/<path>")
        ([this](const std::string& path) {
            if (path.find("..") != std::string::npos) {
                return crow::response(404);
            }
            crow::response res;
            res.set_static_file_info_unsafe(kWebDist_ + "/" + path);
            return res;
        });
    }
};
