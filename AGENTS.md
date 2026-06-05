# AGENTS.md

## 项目说明

`web-template` 是一个**配置管理系统模板**:C++11 httplib 后端 + Vue 2 + Element UI 前端,实现 HTTP Digest 摘要认证保护的配置项增删改查。

- **后端**(`app/`):单头文件 `app.hpp` 实现 `RestApp`(路由 + 持久化)+ `DigestAuth`(自实现,每个路由 handler 开头调用 `checkAuth`)。`main.cpp` ~5 行,只 `RestApp app; app.run();`。
- **前端**(`web/`):Vue 2.7 SPA,Vue Router **hash 模式**,Element UI 表单,axios + blueimp-md5 自实现 digest 客户端。
- **第三方**(`third/`):vendored 头文件 `httplib.h` + `json.hpp` (nlohmann)。无外部依赖,链接仅需 pthread。
- **持久化**(`app/mockdb/` + `app/type/`):typed 模板化 DB。`EasyDB` 把每个 struct 注册成 key,落到 `./db.json`(运行 cwd)。
- **默认凭证**:`admin` / `admin123`,realm `SignalController`。

## 目录结构

```
app/                    # 后端 (C++11 + httplib)
  main.cpp              # ~5 行,只构造并 run RestApp
  app.hpp               # RestApp + DigestAuth + checkAuth + 路由注册 (全在这里)
  md5.hpp               # 独立 MD5,被 DigestAuth 调用
  mockdb/
    easydb.hpp          # EasyDB 模板 + NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT
    namedkey.h          # NAMEDKEY(StructName) 宏,把 struct 注册成 DB key
  type/
    deviceinfo.h        # struct DeviceName, LaneInfo, SzLaneInfoTable 等
  CMakeLists.txt        # C++11,链接 pthread
  build/                # cmake 产物 (gitignored)
third/
  httplib.h             # httplib 单头 (vendored)
  json.hpp              # nlohmann/json 单头 (vendored)
web/                    # 前端 (Vue 2 + Element UI)
  src/
    main.js             # 注册 setOnUnauthorized
    App.vue
    router/index.js     # hash 模式,/login 路由 + beforeEach 守卫
    layouts/AppLayout.vue  # 侧边栏 + 顶栏用户/退出 (菜单自动从路由 meta 渲染)
    views/
      Login.vue
      DeviceName.vue    # 三个字段:deviceName 可编辑,manufacturer/software 禁用
    api/
      auth.js           # digest axios 包装 + 拦截器 + sessionStorage
      device-name.js
  vue.config.js         # devServer port=8081 proxy /api->8080;prod publicPath=/static/
  dist/                 # 打包产物 (gitignored;后端启动时从 ../web/dist/ 提供)
.gitignore              # 已就位
```

## 关键非显然约束

### 后端
- **构建用 C++11**。httplib 纯 C++11 兼容,无需 C++17。
- **httplib 没有中间件机制**。每个路由 handler 开头必须手动调用 `checkAuth(req, res)`,如果返回 false 则直接 return。`app.hpp:138-152` 是 `checkAuth` 实现,`app.hpp:224-237` 是调用示例。
- **静态文件服务自实现**。`serveStaticFile` (`app.hpp:175-221`) 从 `../web/dist/` 读取文件,路径防御 `..` 穿越。
- **路由 `/static/(.*)` 用正则捕获文件路径**。`req.matches[1]` 拿到捕获组,传给 `serveStaticFile`。
- **401 响应 body 带 challenge,不发 `WWW-Authenticate` 头**。原因:浏览器看到 `WWW-Authenticate` 会弹原生对话框并按 realm 缓存 HA1,导致后续请求即使 JS 算的是错误密码也会被浏览器注入的缓存头绕过。`app.hpp:145-150` 是关键。
- **MD5 实现在 `app/md5.hpp`**,独立文件,无外部依赖。**字节序陷阱**:state words (a0/b0/c0/d0) 必须是**顺序**输出 (a0[0..3] b0[0..3] ...),**不能** interleave。`md5.hpp:110-116` 是关键。修改时务必对照 RFC 1321 test vectors。
- **已有两个 API 端点**:`/api/Config/DeviceName` 和 `/api/Config/SzLaneInfoTable`,新增端点需按同模式添加。

### 前端
- **必须用 hash 模式路由**,因为后端 SPA fallback 只服务 `/` 一个路径。`web/src/router/index.js` 走 `mode: 'hash'`。
- **`__digestRetry` 标记**是 axios 拦截器防死循环的关键:第一次 401 后拦截器用 stored creds 算 digest 重试,重试请求**本身**也会触发响应拦截器,如果不标记就再次进入 retry 分支,形成无限循环。`web/src/api/auth.js:117-125`。
- **拦截器优先读 `err.response.data.challenge` (body),回退 `headers['www-authenticate']`**,保持与后端新格式兼容,又不破坏标准客户端。`web/src/api/auth.js:113-115`。
- **凭证存 `sessionStorage["auth.credentials"]`**,关闭标签即清空。`web/src/api/auth.js:4` 启动时 `load()` 自动恢复。
- **dev server 在 8081**,通过 webpack devServer proxy 把 `/api/*` 转到 8080 后端。**生产环境**只有一个 8080,后端同时提供 `/static/*` 和 `/`,SPA 内走 hash 路由。
- **侧边栏菜单** 完全由 `router/index.js` 里 children 路由的 `meta` 自动渲染(`AppLayout.vue:63-72`)。新增子页面只需要在路由里加一个 child 节点。

## 易踩的坑

1. **`NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` 的 from_json 会把缺字段重置成 struct 默认值**(不是上一次持久化的值)。当前 `easydb.hpp:13-14` 用的是这个变体,所以**前端 POST 时必须发全所有字段** —— 如果只发 `{deviceName: "foo"}`,后端 `manufacturer`/`software` 会被覆盖成 `"Hikvision"`/`"V4.0.1"`,丢掉自定义值。`DeviceName.vue` 已经按这个语义实现(load 后 save 带上三字段);新增字段时也要遵守。要保留旧值语义,改回 `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE`。
2. **`db.json` 写在运行 cwd**(`easydb.hpp:21` 写死 `./db.json`),不是相对源码树。后端以哪个目录启动,`db.json` 就落在哪。从 `app/` 跑 `./build/server` 就会在 `app/db.json` 落盘。
3. **不要在 `app/` 留下 `data/` 目录或 `device_name.txt`**。旧版本用过这个文件做持久化,已经废弃;新代码路径完全不读它,留着只会误导。

## 常用命令

```bash
# 后端
cd app && mkdir -p build && cd build && cmake .. && make -j
cd app && ./build/server                              # 跑在 8080
ps -ef | grep "build/server" | grep -v grep | awk '{print $2}' | xargs -r kill -9   # 杀旧进程

# 前端开发 (热重载,需后端先跑)
cd web && npm install
npm run serve                                          # 跑在 8081,代理 /api 到 8080

# 前端生产构建 (产物 web/dist/,后端会自动从 ../web/dist/ 提供)
cd web && npm run build
NODE_ENV=production npm run build                      # 显式指定

# 手工验证 digest 401
curl -i http://127.0.0.1:8080/api/Auth/Check
# 期望: 401,body 为 {"error":"unauthorized","challenge":"Digest ..."}
```

## 改动时的检查清单

1. **改了后端**:`cd app/build && make -j` 重新编译,确认 `app/build/server` mtime 更新。
2. **改了 `app/app.hpp` 路由**:`./app/build/server` 必须重启才生效(先 `kill` 旧进程)。
3. **改了前端**:`cd web && npm run build`,dev 模式 (`npm run serve`) 自动热重载。
4. **新增 API 端点**:
   - `app/type/<name>.h` 加 struct
   - `app/mockdb/easydb.hpp` 顶部加 `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(<Name>, field1, field2, ...)`(或非 WITH_DEFAULT 变体,见"易踩的坑 #1")
   - `app/mockdb/easydb.hpp` 末尾加 `NAMEDKEY(<Name>)`
   - `app.hpp:registerRoutes` 加 `server_.Get("/api/Config/<Name>")` + `server_.Post("/api/Config/<Name>")`,每个 handler 开头调 `checkAuth`
   - 前端 `web/src/api/<name>.js` 暴露函数,**load 后 save 带上所有字段**
   - `router/index.js` 加子路由 (`meta: { requiresAuth: true, title, icon }`),`AppLayout.vue` 侧边栏**自动**渲染
5. **改了默认凭证**:`app/app.hpp:115` 改 `auth_("SignalController", "admin", "admin123")`。
6. **调试 digest 校验失败**:在 `checkAuth` 失败分支加日志,打印 expected vs response 即可。

## 禁止改的东西

- `third/httplib.h` 和 `third/json.hpp` 是 vendored,改前先确认没有更轻的方案。
- `app/main.cpp` 只该有 ~5 行(`#include "app.hpp"` + `int main() { ... }`),所有逻辑塞进 `app.hpp`。
- `app/app.hpp` 不要拆成 .h + .cpp,header-only 是设计要求。
- `md5.hpp` 字节序一旦改对就别再动(参见上文"字节序陷阱")。
- 每个路由 handler 开头必须调用 `checkAuth`,漏了会失去认证保护。