import axios from 'axios'
import md5 from 'blueimp-md5'

const STORAGE_KEY = 'auth.credentials'

let credentials = null

function load() {
    try {
        const raw = sessionStorage.getItem(STORAGE_KEY)
        if (raw) credentials = JSON.parse(raw)
    } catch (e) {
        credentials = null
    }
}

function persist() {
    if (credentials) {
        sessionStorage.setItem(STORAGE_KEY, JSON.stringify(credentials))
    } else {
        sessionStorage.removeItem(STORAGE_KEY)
    }
}

export function setCredentials(username, password) {
    credentials = { username, password }
    persist()
}

export function clearCredentials() {
    credentials = null
    persist()
}

export function hasCredentials() {
    return credentials !== null
}

export function getUsername() {
    return credentials ? credentials.username : null
}

load()

const http = axios.create({
    baseURL: '/api',
    timeout: 10000,
    headers: { 'Content-Type': 'application/json' }
})

function parseChallenge(header) {
    const result = {}
    if (!header) return result
    const body = header.replace(/^Digest\s+/, '')
    const re = /(\w+)\s*=\s*(?:"([^"]*)"|([^,\s]+))/g
    let m
    while ((m = re.exec(body)) !== null) {
        result[m[1]] = m[2] !== undefined ? m[2] : m[3]
    }
    return result
}

function randomCnonce() {
    let s = ''
    const chars = '0123456789abcdef'
    for (let i = 0; i < 16; ++i) s += chars[Math.floor(Math.random() * 16)]
    return s
}

function buildAuthHeader(challenge, config, creds) {
    const method = (config.method || 'get').toUpperCase()
    const uri = config.url

    const ha1 = md5(`${creds.username}:${challenge.realm}:${creds.password}`)
    const ha2 = md5(`${method}:${uri}`)

    const nc = '00000001'
    const cnonce = randomCnonce()

    let response
    if (challenge.qop === 'auth') {
        response = md5(`${ha1}:${challenge.nonce}:${nc}:${cnonce}:auth:${ha2}`)
    } else {
        response = md5(`${ha1}:${challenge.nonce}:${ha2}`)
    }

    const parts = [
        `username="${creds.username}"`,
        `realm="${challenge.realm}"`,
        `nonce="${challenge.nonce}"`,
        `uri="${uri}"`,
        `qop=${challenge.qop}`,
        `nc=${nc}`,
        `cnonce="${cnonce}"`,
        `response="${response}"`
    ]
    if (challenge.opaque) {
        parts.push(`opaque="${challenge.opaque}"`)
    }
    return 'Digest ' + parts.join(', ')
}

let onUnauthorized = null

export function setOnUnauthorized(handler) {
    onUnauthorized = handler
}

http.interceptors.response.use(
    res => res,
    err => {
        const status = err.response && err.response.status
        const headerChallenge = err.response && err.response.headers && err.response.headers['www-authenticate']
        const bodyChallenge = err.response && err.response.data && err.response.data.challenge
        const challenge = headerChallenge || bodyChallenge
        const isDigestChallenge = challenge && challenge.indexOf('Digest') === 0
        const alreadyRetried = err.config && err.config.__digestRetry
        if (status === 401 && isDigestChallenge && credentials && !alreadyRetried) {
            const parsed = parseChallenge(challenge)
            const auth = buildAuthHeader(parsed, err.config, credentials)
            const retry = {
                ...err.config,
                headers: { ...(err.config.headers || {}), Authorization: auth },
                __digestRetry: true
            }
            return http.request(retry).catch(retryErr => {
                if (retryErr.response && retryErr.response.status === 401) {
                    clearCredentials()
                    if (onUnauthorized) onUnauthorized()
                }
                return Promise.reject(retryErr)
            })
        }
        if (status === 401 && onUnauthorized) {
            onUnauthorized()
        }
        return Promise.reject(err)
    }
)

export default http
