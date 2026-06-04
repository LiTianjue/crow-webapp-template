import Vue from 'vue'
import VueRouter from 'vue-router'

import AppLayout from '@/layouts/AppLayout.vue'
import { hasCredentials } from '@/api/auth'

Vue.use(VueRouter)

const routes = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('@/views/Login.vue'),
    meta: { title: '登录' }
  },
  {
    path: '/',
    component: AppLayout,
    redirect: '/device-name',
    children: [
      {
        path: 'device-name',
        name: 'DeviceName',
        component: () => import('@/views/DeviceName.vue'),
        meta: { title: '设备名称', icon: 'el-icon-edit', requiresAuth: true }
      }
    ]
  }
]

const router = new VueRouter({
  mode: 'hash',
  routes
})

router.beforeEach((to, from, next) => {
    if (to.path === '/login') {
        if (hasCredentials()) return next('/')
        return next()
    }
    if (to.matched.some(r => r.meta && r.meta.requiresAuth)) {
        if (!hasCredentials()) {
            return next({ path: '/login', query: { redirect: to.fullPath } })
        }
    }
    next()
})

export default router
