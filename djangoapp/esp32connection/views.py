from django.shortcuts import render

# Create your views here.
from rest_framework.decorators import api_view
from rest_framework.response import Response
from rest_framework import status

@api_view(['POST'])
def receive_uv_data(request):
    data = request.data
    print("Received:", data)
    return Response({"message": "Data received"}, status=status.HTTP_201_CREATED)
