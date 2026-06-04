# Web 前端 (Vue 2 + Element UI)

为 C++ 后端 (`../app/`) 提供配置管理 UI。当前实现:

- 左侧固定 200px 导航栏
- 每个配置项一个独立页面
- `GET` 加载,`POST` 保存
- 设备名称页面 `views/DeviceName.vue`
- HTTP Digest 摘要认证 (`src/api/auth.js`),默认 `admin` / `admin123`
- 登录页 (`src/views/Login.vue`),路由守卫 `requiresAuth`

## 技术栈

| 层次 | 技术 |
|------|------|
| 框架 | Vue 2.7 + Vue Router 3 |
| UI | Element UI 2.15 |
| HTTP | axios 1.6 |
| 加密 | blueimp-md5 2.19 (digest 响应计算) |
| 构建 | @vue/cli-service 5 |

## 安装

```bash
cd web
npm install
```

## 开发 (热重载, 端口 8081)

```bash
# 后端先启动: cd ../app && ./build/server
npm run serve
```

开发服务器: <http://localhost:8081>
- `/api/*` 请求被代理到 `http://127.0.0.1:8080` (后端)
- 前端代码改动实时刷新

## 生产构建

```bash
npm run build
```

构建产物输出到本地 `dist/`:
- `dist/index.html` — 入口 HTML
- `dist/js/*.js` — 打包后的 JS
- `dist/css/*.css` — 打包后的 CSS

构建使用 `publicPath=/static/`, 引用形如 `/static/js/app.xxx.js`。
后端 (`../app/main.cpp`) 启动后会自动从 `../web/dist/` 提供这些静态资源:
- `GET /` → `../web/dist/index.html`
- `GET /static/<path>` → `../web/dist/<path>`

## 认证流程

1. 用户访问任意非 `/login` 路由,路由守卫 (`router.beforeEach`) 检查
   `sessionStorage` 中是否有凭证。没有则跳 `/login?redirect=<原 URL>`。
2. 用户在登录页输入用户名密码,提交后:
   - `setCredentials(u, p)` 写入 `sessionStorage["auth.credentials"]`
   - `GET /api/Auth/Check` 触发第一次 401,axios 拦截器收到 `WWW-Authenticate: Digest ...`
   - 拦截器用 `blueimp-md5` 计算 `HA1/HA2/response`,自动重发带 `Authorization: Digest ...` 的请求
   - 服务端校验通过 → 200,前端跳到来源 URL (默认 `/device-name`)
3. 后续所有 `/api/*` 请求经 axios 拦截器自动加 digest 头。sessionStorage 关闭标签页即清空。
4. 错误密码重试仍 401 → 拦截器清除凭证 + 触发 `onUnauthorized` → 跳回 `/login`,UI 显示"用户名或密码错误"。
5. 顶栏右侧下拉菜单提供"退出登录"。

## 目录结构

```
web/
  package.json
  vue.config.js          # 端口 / 代理 / publicPath
  babel.config.js
  public/
    index.html
  src/
    main.js              # 全局 onUnauthorized 注册
    App.vue
    router/index.js      # /login + 路由守卫
    layouts/AppLayout.vue # 侧边栏 + 顶栏用户/退出
    views/
      Login.vue          # 登录页
      DeviceName.vue     # 设备名称配置
    api/
      auth.js            # digest 认证 axios 包装
      device-name.js
  dist/                  # 打包产物 (npm run build)
```
