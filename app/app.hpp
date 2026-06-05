#pragma once

#include "../third/httplib.h"
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

    bool verify(const httplib::Request& req, const std::string& method) const {
        auto auth_it = req.headers.find("Authorization");
        if (auth_it == req.headers.end()) {
            return false;
        }
        const std::string& auth = auth_it->second;
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
        const std::string uri      = get("uri").empty() ? req.path : get("uri");
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

class RestApp {
public:
    RestApp()
        : auth_("SignalController", "admin", "admin123") {
        registerRoutes();
    }

    RestApp(const RestApp&) = delete;
    RestApp& operator=(const RestApp&) = delete;

    void run(uint16_t port = 8080) {
        server_.listen("0.0.0.0", port);
    }

private:
    httplib::Server server_;
    DigestAuth auth_;
    EasyDB db;
    const std::string kWebDist_     = "../web/dist";

    void setJsonResponse(httplib::Response& res, int status, const json& body) {
        res.status = status;
        res.set_header("Content-Type", "application/json");
        res.body = body.dump();
    }

    bool checkAuth(const httplib::Request& req, httplib::Response& res) {
        if (req.path.compare(0, 5, "/api/") != 0) {
            return true;
        }
        if (auth_.verify(req, req.method)) {
            return true;
        }
        res.status = 401;
        res.set_header("Content-Type", "application/json");
        res.body = json{
            {"error", "unauthorized"},
            {"challenge", auth_.challenge()}
        }.dump();
        return false;
    }

    template <typename T>
    void renderPOST(const httplib::Request& req, httplib::Response& res) {
        T t;
        json j = json::parse(req.body, nullptr, false);
        if (!j.is_discarded()) {
            j.get_to(t);
        }
        db.write(t);
        res.status = 200;
        res.body = "OK";
    }

    template <typename T>
    void renderGET(const httplib::Request& req, httplib::Response& res) {
        T t;
        db.read(t);
        res.status = 200;
        res.set_header("Content-Type", "application/json");
        res.body = json(t).dump();
    }

    bool serveStaticFile(const std::string& path, httplib::Response& res) {
        if (path.find("..") != std::string::npos) {
            res.status = 404;
            return false;
        }
        std::string fullPath = kWebDist_ + "/" + path;
        std::ifstream in(fullPath, std::ios::binary | std::ios::ate);
        if (!in) {
            res.status = 404;
            return false;
        }
        std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        in.read(buffer.data(), size);
        if (!in) {
            res.status = 404;
            return false;
        }
        res.status = 200;
        res.body.assign(buffer.data(), size);
        std::string ext;
        size_t dotPos = path.rfind('.');
        if (dotPos != std::string::npos) {
            ext = path.substr(dotPos + 1);
        }
        if (ext == "html" || ext == "htm") {
            res.set_header("Content-Type", "text/html");
        } else if (ext == "js") {
            res.set_header("Content-Type", "application/javascript");
        } else if (ext == "css") {
            res.set_header("Content-Type", "text/css");
        } else if (ext == "png") {
            res.set_header("Content-Type", "image/png");
        } else if (ext == "jpg" || ext == "jpeg") {
            res.set_header("Content-Type", "image/jpeg");
        } else if (ext == "ico") {
            res.set_header("Content-Type", "image/x-icon");
        } else if (ext == "woff") {
            res.set_header("Content-Type", "font/woff");
        } else if (ext == "woff2") {
            res.set_header("Content-Type", "font/woff2");
        } else {
            res.set_header("Content-Type", "application/octet-stream");
        }
        return true;
    }

    void registerRoutes() {
        server_.Get("/api/Auth/Check", [&](const httplib::Request& req, httplib::Response& res) {
            if (!checkAuth(req, res)) return;
            setJsonResponse(res, 200, json{{"user", auth_.user()}});
        });

        server_.Get("/api/Config/DeviceName", [&](const httplib::Request& req, httplib::Response& res) {
            if (!checkAuth(req, res)) return;
            renderGET<DeviceName>(req, res);
        });

        server_.Post("/api/Config/DeviceName", [&](const httplib::Request& req, httplib::Response& res) {
            if (!checkAuth(req, res)) return;
            renderPOST<DeviceName>(req, res);
        });

        server_.Get("/api/Config/SzLaneInfoTable", [&](const httplib::Request& req, httplib::Response& res) {
            if (!checkAuth(req, res)) return;
            renderGET<SzLaneInfoTable>(req, res);
        });

        server_.Post("/api/Config/SzLaneInfoTable", [&](const httplib::Request& req, httplib::Response& res) {
            if (!checkAuth(req, res)) return;
            renderPOST<SzLaneInfoTable>(req, res);
        });

        server_.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
            serveStaticFile("index.html", res);
        });

        server_.Get("/static/(.*)", [&](const httplib::Request& req, httplib::Response& res) {
            serveStaticFile(req.matches[1], res);
        });
    }
};