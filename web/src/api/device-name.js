import http from './auth'

export const deviceNameApi = {
    get() {
        return http.get('/Config/DeviceName')
    },
    update(data) {
        return http.post('/Config/DeviceName', data)
    }
}
