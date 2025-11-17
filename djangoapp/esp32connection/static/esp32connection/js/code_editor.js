// Monaco Editor configuration
let editor = null;
let currentCodeId = null;
let codes = [];

// Initialize Monaco Editor
require.config({ paths: { vs: 'https://cdn.jsdelivr.net/npm/monaco-editor@0.45.0/min/vs' } });
require(['vs/editor/editor.main'], function () {
    editor = monaco.editor.create(document.getElementById('editor'), {
        value: getDefaultCode(),
        language: 'cpp',
        theme: 'vs-dark',
        automaticLayout: true,
        fontSize: 14,
        minimap: { enabled: true },
        scrollBeyondLastLine: false,
        wordWrap: 'on',
    });

    // Load code list
    loadCodeList();
});

// Get default code template
function getDefaultCode(language = 'cpp') {
    const templates = {
        cpp: `// ESP32 Arduino Code Template
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "http://your-server.com/esp32connection/uv/";

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\\nWiFi connected!");
    
    // Initialize sensors, etc.
    // TODO: Add your initialization code
}

void loop() {
    // Read sensor data
    float uv_index = 0.0; // TODO: Read actual value from sensor
    
    // Send data to server
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");
        
        String jsonData = "{\\"device_id\\":\\"esp32-1\\",\\"uv_index\\":" + String(uv_index) + ",\\"timestamp\\":\\"" + getTimestamp() + "\\"}";
        
        int httpResponseCode = http.POST(jsonData);
        if (httpResponseCode > 0) {
            Serial.println("Data sent successfully");
        } else {
            Serial.println("Error sending data");
        }
        http.end();
    }
    
    delay(30000); // Send every 30 seconds
}

String getTimestamp() {
    // TODO: Implement timestamp generation
    return "2025-01-01T00:00:00Z";
}`,
        c: `// ESP32 C Code Template
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
    printf("ESP32 C Code Template\\n");
    // TODO: Add your code
}`,
        python: `# ESP32 MicroPython Code Template
from machine import Pin
import time
import network
import urequests

# WiFi Configuration
WIFI_SSID = "YOUR_WIFI_SSID"
WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"
SERVER_URL = "http://your-server.com/esp32connection/uv/"

def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(WIFI_SSID, WIFI_PASSWORD)
    
    while not wlan.isconnected():
        time.sleep(1)
    print("WiFi connected!")

def send_data(uv_index):
    try:
        data = {
            "device_id": "esp32-1",
            "uv_index": uv_index,
            "timestamp": "2025-01-01T00:00:00Z"
        }
        response = urequests.post(SERVER_URL, json=data)
        response.close()
        print("Data sent successfully")
    except Exception as e:
        print(f"Error: {e}")

def main():
    connect_wifi()
    
    while True:
        # TODO: Read sensor data
        uv_index = 0.0
        
        send_data(uv_index)
        time.sleep(30)  # Send every 30 seconds

if __name__ == "__main__":
    main()`
    };
    return templates[language] || templates.cpp;
}

// Load code list
async function loadCodeList() {
    try {
        const response = await fetch(LIST_CODES_URL);
        if (!response.ok) throw new Error('Failed to load codes');
        
        codes = await response.json();
        renderFileList();
        
        if (codes.length > 0) {
            loadCode(codes[0].id);
        } else {
            newFile();
        }
    } catch (error) {
        console.error('Error loading codes:', error);
        showStatus('Failed to load code list: ' + error.message, 'error');
        renderFileList();
    }
}

// Render file list
function renderFileList() {
    const fileList = document.getElementById('file-list');
    
    if (codes.length === 0) {
        fileList.innerHTML = `
            <div class="empty-state">
                <p>No code files yet</p>
                <button class="btn btn-primary" onclick="newFile()">Create first file</button>
            </div>
        `;
        return;
    }
    
    fileList.innerHTML = codes.map(code => `
        <div class="file-item ${code.id === currentCodeId ? 'active' : ''}" onclick="loadCode(${code.id})">
            <span class="file-item-name">${escapeHtml(code.name)}</span>
            <span class="file-item-language">${getLanguageLabel(code.language)}</span>
            <button class="file-item-delete" onclick="event.stopPropagation(); deleteCode(${code.id})">Delete</button>
        </div>
    `).join('');
}

// Load code
async function loadCode(codeId) {
    try {
        const response = await fetch(`${API_BASE}/api/codes/${codeId}/`);
        if (!response.ok) throw new Error('Failed to load code');
        
        const code = await response.json();
        currentCodeId = code.id;
        document.getElementById('file-name').value = code.name;
        document.getElementById('language-select').value = code.language;
        document.getElementById('delete-btn').style.display = 'block';
        
        if (editor) {
            editor.setValue(code.code);
            monaco.editor.setModelLanguage(editor.getModel(), getMonacoLanguage(code.language));
        }
        
        renderFileList();
        showStatus('Code loaded successfully', 'success');
    } catch (error) {
        console.error('Error loading code:', error);
        showStatus('Failed to load code: ' + error.message, 'error');
    }
}

// Save code
async function saveCode() {
    const name = document.getElementById('file-name').value.trim();
    if (!name) {
        showStatus('Please enter a file name', 'error');
        return;
    }
    
    const code = editor.getValue();
    const language = document.getElementById('language-select').value;
    
    try {
        const response = await fetch(SAVE_CODE_URL, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-CSRFToken': getCsrfToken(),
            },
            body: JSON.stringify({
                id: currentCodeId,
                name: name,
                code: code,
                language: language,
            }),
        });
        
        if (!response.ok) throw new Error('Failed to save code');
        
        const result = await response.json();
        currentCodeId = result.id;
        showStatus('Code saved successfully', 'success');
        loadCodeList();
    } catch (error) {
        console.error('Error saving code:', error);
        showStatus('Failed to save code: ' + error.message, 'error');
    }
}

// Delete code
async function deleteCode(codeId) {
    if (!confirm('Are you sure you want to delete this file?')) {
        return;
    }
    
    try {
        const response = await fetch(`${API_BASE}/api/codes/${codeId}/delete/`, {
            method: 'DELETE',
            headers: {
                'X-CSRFToken': getCsrfToken(),
            },
        });
        
        if (!response.ok) throw new Error('Failed to delete code');
        
        showStatus('Code deleted successfully', 'success');
        
        if (currentCodeId === codeId) {
            currentCodeId = null;
            newFile();
        }
        
        loadCodeList();
    } catch (error) {
        console.error('Error deleting code:', error);
        showStatus('Failed to delete code: ' + error.message, 'error');
    }
}

// New file
function newFile() {
    currentCodeId = null;
    document.getElementById('file-name').value = '';
    document.getElementById('language-select').value = 'cpp';
    document.getElementById('delete-btn').style.display = 'none';
    
    if (editor) {
        editor.setValue(getDefaultCode('cpp'));
        monaco.editor.setModelLanguage(editor.getModel(), 'cpp');
    }
    
    renderFileList();
    showStatus('New file created', 'success');
}

// Show status
function showStatus(message, type = '') {
    const statusBar = document.querySelector('.status-bar');
    const statusText = document.getElementById('status-text');
    
    statusText.textContent = message;
    statusBar.className = 'status-bar ' + type;
    
    setTimeout(() => {
        statusBar.className = 'status-bar';
        statusText.textContent = 'Ready';
    }, 3000);
}

// Utility functions
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function getLanguageLabel(language) {
    const labels = {
        'cpp': 'C++',
        'c': 'C',
        'python': 'Python'
    };
    return labels[language] || language;
}

function getMonacoLanguage(language) {
    const mapping = {
        'cpp': 'cpp',
        'c': 'c',
        'python': 'python'
    };
    return mapping[language] || 'cpp';
}

function getCsrfToken() {
    const cookies = document.cookie.split(';');
    for (let cookie of cookies) {
        const [name, value] = cookie.trim().split('=');
        if (name === 'csrftoken') {
            return value;
        }
    }
    return '';
}

// Copy code to clipboard
function copyCode() {
    if (!editor) {
        showStatus('Editor not ready', 'error');
        return;
    }
    
    const code = editor.getValue();
    navigator.clipboard.writeText(code).then(() => {
        showStatus('Code copied to clipboard!', 'success');
    }).catch(err => {
        // Fallback for older browsers
        const textArea = document.createElement('textarea');
        textArea.value = code;
        textArea.style.position = 'fixed';
        textArea.style.left = '-999999px';
        document.body.appendChild(textArea);
        textArea.select();
        try {
            document.execCommand('copy');
            showStatus('Code copied to clipboard!', 'success');
        } catch (err) {
            showStatus('Failed to copy code', 'error');
        }
        document.body.removeChild(textArea);
    });
}

// Download code as file
function downloadCode() {
    if (!editor) {
        showStatus('Editor not ready', 'error');
        return;
    }
    
    const code = editor.getValue();
    const fileName = document.getElementById('file-name').value.trim() || 'esp32_code';
    const language = document.getElementById('language-select').value;
    
    // Determine file extension
    const extensions = {
        'cpp': '.ino',
        'c': '.c',
        'python': '.py'
    };
    const extension = extensions[language] || '.txt';
    
    const blob = new Blob([code], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = fileName + extension;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
    
    showStatus('Code downloaded!', 'success');
}

// Event listeners
document.getElementById('save-btn').addEventListener('click', saveCode);
document.getElementById('copy-btn').addEventListener('click', copyCode);
document.getElementById('download-btn').addEventListener('click', downloadCode);
document.getElementById('new-file-btn').addEventListener('click', newFile);
document.getElementById('delete-btn').addEventListener('click', () => {
    if (currentCodeId) {
        deleteCode(currentCodeId);
    }
});

document.getElementById('language-select').addEventListener('change', (e) => {
    if (editor && !currentCodeId) {
        const language = e.target.value;
        editor.setValue(getDefaultCode(language));
        monaco.editor.setModelLanguage(editor.getModel(), getMonacoLanguage(language));
    }
});

// Keyboard shortcut for save (Ctrl+S / Cmd+S)
document.addEventListener('keydown', (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key === 's') {
        e.preventDefault();
        saveCode();
    }
});

