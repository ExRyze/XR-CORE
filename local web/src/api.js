(function (global) {
    'use strict';

    const API = {
        system: {
            getStatus: {
                method: 'GET',
                path: '/api/system/status',
                source: 'ESPSystem',
                description: '',
                fields: [
                    'chipModel', 'chipRevision', 'chipCores', 'cpuFreqMHz',
                    'heapSize', 'freeHeap', 'minFreeHeap', 'maxAllocHeap',
                    'flashChipSize', 'flashChipSpeed', 'flashChipMode',
                    'flashFrequencyMHz', 'sdkVersion', 'coreVersion',
                    'efuseMac', 'sketchSize', 'sketchMD5', 'freeSketchSpace'
                ]
            },
            restart: {
                method: 'POST',
                path: '/api/system/restart',
                source: 'ESPSystem.restart()',
                description: ''
            },
        },

        device: {
            getInfo: {
                method: 'GET',
                path: '/api/device/status',
                source: 'Config.h variables',
                description: '',
                fields: ['brand', 'name', 'version']
            },
        },

        wifi: {
            getStatus: {
                method: 'GET',
                path: '/api/wifi/status',
                source: 'WiFiManager.getMode(), getIp(), getSavedSsid()',
                description: '',
                fields: ['mode', 'ip', 'savedSsid', 'isConnected', 'rssi', 'signalQuality', 'time']
            },
            scan: {
                method: 'GET',
                path: '/api/wifi/scan',
                source: 'WiFiManager.scanNetworks(), getScanCount(), getScanSsid(), getScanRssi()',
                description: '',
                fields: ['ssid', 'rssi', 'encryptionType', 'channel']
            },
            save: {
                method: 'POST',
                path: '/api/wifi/save',
                source: 'WiFiManager.saveCredentials()',
                description: '',
                params: { ssid: 'string', password: 'string', restartAfterSave: 'boolean' }
            },
            clear: {
                method: 'POST',
                path: '/api/wifi/clear',
                source: 'WiFiManager.clearCredentials()',
                description: '',
                params: {}
            },
            clearApPassword: {
                method: 'POST',
                path: '/api/wifi/ap-password/clear',
                source: 'WiFiManager.clearApPassword()',
                description: '',
                params: {}
            },
        },

        mqtt: {
            async getSettings() {
                return requestJson('/api/mqtt/settings');
            },
            async getStatus() {
                return requestJson('/api/mqtt/status');
            },
            async clearSettings() {
                return requestJson('/api/mqtt/settings/reset', {
                    method: 'POST',
                    headers: { 'X-Requested-With': 'XMLHttpRequest' }
                });
            },
            async saveSettings(settings) {
                const params = new URLSearchParams({
                    enabled: String(settings.enabled),
                    server: settings.server,
                    port: String(settings.port),
                    clientId: settings.clientId,
                    baseTopic: settings.baseTopic,
                    username: settings.username,
                    password: settings.password,
                    clearPassword: String(settings.clearPassword),
                    tls: String(settings.tls)
                });
                return requestJson('/api/mqtt/settings', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8',
                        'X-Requested-With': 'XMLHttpRequest'
                    },
                    body: params.toString()
                });
            },
            async getCaStatus() {
                return requestJson('/api/mqtt/ca/status');
            },
            async uploadCaCertificate(file) {
                const formData = new FormData();
                formData.append('certificate', file);
                return requestJson('/api/mqtt/ca/upload', {
                    method: 'POST',
                    headers: { 'X-Requested-With': 'XMLHttpRequest' },
                    body: formData
                });
            }
        },

        lora: {
            async getSettings() {
                return requestJson('/api/lora/settings');
            },
            async clearSettings() {
                return requestJson('/api/lora/settings/reset', {
                    method: 'POST',
                    headers: { 'X-Requested-With': 'XMLHttpRequest' }
                });
            },
            async saveSettings(settings) {
                const params = new URLSearchParams({
                    enabled: String(settings.enabled),
                    frequency: String(settings.frequency),
                    bandwidth: String(settings.bandwidth),
                    spreadingFactor: String(settings.spreadingFactor),
                    codingRate: String(settings.codingRate),
                    txPower: String(settings.txPower),
                    syncWord: settings.syncWord,
                    preambleLength: String(settings.preambleLength),
                    crcEnabled: String(settings.crcEnabled),
                    nodeId: settings.nodeId
                });
                return requestJson('/api/lora/settings', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8',
                        'X-Requested-With': 'XMLHttpRequest'
                    },
                    body: params.toString()
                });
            }
        },

    };

    async function requestJson(path, options = {}) {
        const { timeoutMs = 5000, signal, ...requestOptions } = options;
        const headers = { ...(options.headers || {}) };
        if (!(options.body instanceof FormData) && !headers['Content-Type']) {
            headers['Content-Type'] = 'application/json';
        }
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), timeoutMs);
        if (signal) {
            if (signal.aborted) controller.abort();
            else signal.addEventListener('abort', () => controller.abort(), { once: true });
        }

        const finalOptions = {
            ...requestOptions,
            signal: controller.signal,
            headers
        };

        let response;
        let text;
        try {
            response = await fetch(path, finalOptions);
            text = await response.text();
        } catch (error) {
            if (controller.signal.aborted) {
                throw new Error(`Request timed out: ${path}`);
            }
            throw error;
        } finally {
            clearTimeout(timeoutId);
        }

        if (!response.ok) {
            const message = text || `Request failed with status ${response.status}`;
            throw new Error(message);
        }

        if (!text) {
            return null;
        }

        try {
            return JSON.parse(text);
        } catch (error) {
            return text;
        }
    }

    const DeviceApi = {
        list: API,

        fetch: requestJson,

        system: {
            async getStatus() {
                return requestJson('/api/system/status');
            },
            async restart() {
                return requestJson('/api/system/restart', {
                    method: 'POST',
                    headers: { 'X-Requested-With': 'XMLHttpRequest' }
                });
            },
        },

        device: {
            async getStatus() {
                return requestJson('/api/device/status');
            }
        },

        wifi: {
            async getStatus() {
                return requestJson('/api/wifi/status');
            },
            async save(ssid, password, restartAfterSave = false) {
                const params = new URLSearchParams({ ssid, password, restartAfterSave: String(restartAfterSave) });
                return requestJson('/api/wifi/save', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded; charset=UTF-8',
                        'X-Requested-With': 'XMLHttpRequest'
                    },
                    body: params.toString()
                });
            },
            async scan() {
                return requestJson('/api/wifi/scan');
            },
            async clear() {
                return requestJson('/api/wifi/clear', {
                    method: 'POST',
                    headers: { 'X-Requested-With': 'XMLHttpRequest' }
                });
            },
            async clearApPassword() {
                return requestJson('/api/wifi/ap-password/clear', {
                    method: 'POST',
                    headers: { 'X-Requested-With': 'XMLHttpRequest' }
                });
            }
        },
        mqtt: API.mqtt,
        lora: API.lora,
    };

    global.DeviceApi = DeviceApi;
    global.DEVICE_API = API;
})(window);
