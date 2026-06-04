<template>
  <el-container class="layout">
    <el-aside width="200px" class="aside">
      <div class="logo">配置管理</div>
      <el-menu
        :default-active="$route.path"
        :router="true"
        background-color="#001529"
        text-color="#ffffffcc"
        active-text-color="#409EFF"
      >
        <el-menu-item
          v-for="item in menuItems"
          :key="item.path"
          :index="item.path"
        >
          <i :class="item.icon"></i>
          <span slot="title">{{ item.title }}</span>
        </el-menu-item>
      </el-menu>
    </el-aside>

    <el-container>
      <el-header class="header">
        <span class="title">{{ currentTitle }}</span>
        <div class="spacer"></div>
        <el-dropdown trigger="click" @command="onCommand">
          <span class="user">
            <i class="el-icon-user-solid"></i>
            <span>{{ username || '用户' }}</span>
            <i class="el-icon-arrow-down"></i>
          </span>
          <el-dropdown-menu slot="dropdown">
            <el-dropdown-item command="logout">退出登录</el-dropdown-item>
          </el-dropdown-menu>
        </el-dropdown>
      </el-header>
      <el-main class="main">
        <router-view v-slot="{ Component }">
          <transition name="fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </el-main>
    </el-container>
  </el-container>
</template>

<script>
import { getUsername, clearCredentials } from '@/api/auth'

export default {
  name: 'AppLayout',
  data() {
    return {
      username: getUsername()
    }
  },
  computed: {
    currentTitle() {
      return this.$route.meta.title || '配置管理'
    },
    menuItems() {
      return this.$router.options.routes
        .find(r => r.path === '/')
        .children
        .filter(r => r.path && r.meta)
        .map(r => ({
          path: '/' + r.path,
          title: r.meta.title,
          icon: r.meta.icon || 'el-icon-setting'
        }))
    }
  },
  methods: {
    onCommand(cmd) {
      if (cmd === 'logout') {
        clearCredentials()
        this.$router.push('/login')
      }
    }
  }
}
</script>

<style scoped>
.layout { height: 100vh; }
.aside { background-color: #001529; }
.logo {
  height: 60px;
  line-height: 60px;
  text-align: center;
  color: #fff;
  font-size: 16px;
  font-weight: 600;
  background-color: #002140;
  letter-spacing: 2px;
}
.aside >>> .el-menu {
  border-right: none;
}
.header {
  background-color: #fff;
  border-bottom: 1px solid #e8e8e8;
  display: flex;
  align-items: center;
  padding: 0 20px;
}
.title {
  font-size: 16px;
  font-weight: 500;
  color: #303133;
}
.spacer { flex: 1; }
.user {
  display: flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
  color: #606266;
  font-size: 14px;
}
.user:hover { color: #409EFF; }
.main {
  background-color: #f0f2f5;
  padding: 20px;
}
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.15s ease;
}
.fade-enter,
.fade-leave-to {
  opacity: 0;
}
</style>
