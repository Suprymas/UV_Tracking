import requests
r = requests.post(
    "http://127.0.0.1:8000/api/uv/",
    json={"device_id":"esp32-1","uv_index":3.7,"timestamp":"2025-10-29T13:45:00Z"}
)
print(r.status_code, r.text)
