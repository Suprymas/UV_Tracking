import requests
from celery import shared_task
from django.conf import settings
from .models import UVData


@shared_task
def fetch_uv_data():
    api_key = settings.UV_API_KEY
    latitude = settings.UV_LAT
    longitude = settings.UV_LNG

    url = "https://api.openuv.io/api/v1/uv"
    headers = {
        "x-access-token": api_key,
        "Content-Type": "application/json"
    }
    params = {
        "lat": latitude,
        "lng": longitude,
    }

    try:
        response = requests.get(url, headers=headers, params=params)
        response.raise_for_status()
        data = response.json()

        uv_value = data["result"]["uv"]

        # r = redis.Redis(host=settings.REDIS_HOST, port=settings.REDIS_PORT, db=0)
        # r.set('latest_uv', uv_value)
        UVData.objects.create(
            uv_value=uv_value,
        )

        return {'success': True, 'uv': uv_value}

    except requests.exceptions.HTTPError as e:
        print(f"Error: {e}")
        return {'success': False, 'error': str(e)}