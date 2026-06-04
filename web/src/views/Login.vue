<template>
  <div class="login-page">
    <el-card class="login-card" shadow="always">
      <div slot="header" class="login-header">
        <span class="title">配置管理</span>
        <span class="subtitle">SignalController</span>
      </div>

      <el-form
        ref="form"
        :model="form"
        :rules="rules"
        label-position="top"
        @submit.native.prevent="onSubmit"
        :disabled="loading"
      >
        <el-form-item label="用户名" prop="username">
          <el-input
            v-model="form.username"
            placeholder="admin"
            autocomplete="username"
            clearable
            autofocus
          />
        </el-form-item>
        <el-form-item label="密码" prop="password">
          <el-input
            v-model="form.password"
            type="password"
            placeholder="admin123"
            autocomplete="current-password"
            show-password
            @keyup.enter.native="onSubmit"
          />
        </el-form-item>
        <el-form-item v-if="error" class="error-item">
          <el-alert :title="error" type="error" :closable="false" show-icon />
        </el-form-item>
        <el-form-item>
          <el-button
            type="primary"
            :loading="loading"
            @click="onSubmit"
            style="width: 100%"
          >登录</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script>
import http, { setCredentials, clearCredentials } from '@/api/auth'

export default {
  name: 'Login',
  data() {
    return {
      loading: false,
      error: '',
      form: {
        username: '',
        password: ''
      },
      rules: {
        username: [{ required: true, message: '请输入用户名', trigger: 'blur' }],
        password: [{ required: true, message: '请输入密码', trigger: 'blur' }]
      }
    }
  },
  methods: {
    onSubmit() {
      this.error = ''
      this.$refs.form.validate(valid => {
        if (!valid) return
        this.loading = true
        setCredentials(this.form.username, this.form.password)
        http.get('/Auth/Check')
          .then(res => {
            const redirect = this.$route.query.redirect || '/device-name'
            this.$router.replace(redirect)
          })
          .catch(err => {
            clearCredentials()
            const status = err.response && err.response.status
            if (status === 401) {
              this.error = '用户名或密码错误'
            } else {
              this.error = '无法连接服务器: ' + (err.message || '网络错误')
            }
          })
          .finally(() => { this.loading = false })
      })
    }
  }
}
</script>

<style scoped>
.login-page {
  position: fixed;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #001529 0%, #003a8c 100%);
}
.login-card {
  width: 380px;
}
.login-header {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
}
.title {
  font-size: 20px;
  font-weight: 600;
  color: #303133;
}
.subtitle {
  font-size: 12px;
  color: #909399;
}
.error-item {
  margin-bottom: 16px;
}
</style>
