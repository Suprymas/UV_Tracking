from django.shortcuts import render

# Create your views here.
from rest_framework.decorators import api_view
from rest_framework.response import Response
from rest_framework import status
import json

@api_view(['POST'])
def receive_uv_data(request):
    try:
        data = request.data
        device_id = data.get('device_id')
        uv_index = data.get('uv_index')
        timestamp = data.get('timestamp')
        print(f"Received data from {device_id}: UV={uv_index}, time={timestamp}")
        return Response({"message": "Data received successfully"}, status=status.HTTP_201_CREATED)
    except Exception as e:
        return Response({"error": str(e)}, status=status.HTTP_400_BAD_REQUEST)
    