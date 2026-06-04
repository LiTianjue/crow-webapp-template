<template>
  <div class="form-container">
    <el-card shadow="never">
      <div slot="header" class="card-header">
        <span>设备名称配置</span>
        <el-button
          type="text"
          icon="el-icon-refresh"
          @click="load"
          :loading="loading"
        >刷新</el-button>
      </div>

      <el-form
        ref="form"
        :model="form"
        :rules="rules"
        label-width="100px"
        v-loading="loading"
        @submit.native.prevent
      >
        <el-form-item label="设备名称" prop="deviceName">
          <el-input
            v-model="form.deviceName"
            placeholder="请输入设备名称"
            clearable
            maxlength="64"
            show-word-limit
          />
        </el-form-item>

        <el-form-item label="制造商" prop="manufacturer">
          <el-input
            v-model="form.manufacturer"
            placeholder="制造商"
            clearable
            maxlength="64"
            show-word-limit
            :disabled="true"
          />
        </el-form-item>

        <el-form-item label="软件版本" prop="software">
          <el-input
            v-model="form.software"
            placeholder="软件版本"
            clearable
            maxlength="64"
            show-word-limit
            :disabled="true"
          />
        </el-form-item>

        <el-form-item>
          <el-button
            type="primary"
            :loading="saving"
            @click="onSave"
          >保存</el-button>
          <el-button @click="onReset">重置</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script>
import { deviceNameApi } from '@/api/device-name'

export default {
  name: 'DeviceNameConfig',
  data() {
    return {
      loading: false,
      saving: false,
      form: {
        deviceName: '',
        manufacturer: '',
        software: ''
      },
      rules: {
        deviceName: [
          { required: true, message: '请输入设备名称', trigger: 'blur' },
          { whitespace: true, message: '设备名称不能为空', trigger: 'blur' },
          { max: 64, message: '长度不能超过 64 个字符', trigger: 'blur' }
        ]
      }
    }
  },
  created() {
    this.load()
  },
  methods: {
    load() {
      this.loading = true
      deviceNameApi.get()
        .then(res => {
          const data = (res && res.data) || {}
          this.form.deviceName   = data.deviceName   || ''
          this.form.manufacturer = data.manufacturer || ''
          this.form.software     = data.software     || ''
        })
        .catch(err => {
          this.$message.error(this.errorMessage(err, '加载失败'))
        })
        .finally(() => { this.loading = false })
    },
    onSave() {
      this.$refs.form.validate(valid => {
        if (!valid) return
        this.saving = true
        deviceNameApi.update({
          deviceName:   this.form.deviceName,
          manufacturer: this.form.manufacturer,
          software:     this.form.software
        })
          .then(res => {
            this.$message.success('保存成功')
            const data = (res && res.data) || {}
            this.form.deviceName   = data.deviceName   || this.form.deviceName
            this.form.manufacturer = data.manufacturer || this.form.manufacturer
            this.form.software     = data.software     || this.form.software
          })
          .catch(err => {
            this.$message.error(this.errorMessage(err, '保存失败'))
          })
          .finally(() => { this.saving = false })
      })
    },
    onReset() {
      this.$refs.form.clearValidate()
      this.load()
    },
    errorMessage(err, fallback) {
      return (err.response && err.response.data && err.response.data.error) || err.message || fallback
    }
  }
}
</script>

<style scoped>
.form-container {
  max-width: 720px;
}
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>
