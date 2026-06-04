import Vue from 'vue'
import ElementUI from 'element-ui'
import 'element-ui/lib/theme-chalk/index.css'

import App from './App.vue'
import router from './router'
import { setOnUnauthorized, hasCredentials } from './api/auth'

Vue.use(ElementUI, { size: 'small' })
Vue.config.productionTip = false

setOnUnauthorized(() => {
    if (router.currentRoute.path !== '/login') {
        router.replace({ path: '/login', query: { redirect: router.currentRoute.fullPath } })
    }
})

new Vue({
    router,
    render: h => h(App)
}).$mount('#app')
