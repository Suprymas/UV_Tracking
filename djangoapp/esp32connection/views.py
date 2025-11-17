from django.shortcuts import render, get_object_or_404
from django.http import JsonResponse
from django.views.decorators.http import require_http_methods
from django.views.decorators.csrf import csrf_exempt
from django.utils import timezone

# Create your views here.
from rest_framework.decorators import api_view
from rest_framework.response import Response
from rest_framework import status
from .models import ESP32Code
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


def code_editor(request):
    """Code editor page"""
    return render(request, 'esp32connection/code_editor.html')


@api_view(['GET'])
def list_codes(request):
    """Get all code files list"""
    codes = ESP32Code.objects.all()
    data = [{
        'id': code.id,
        'name': code.name,
        'language': code.language,
        'description': code.description,
        'created_at': code.created_at.isoformat(),
        'updated_at': code.updated_at.isoformat(),
    } for code in codes]
    return Response(data)


@api_view(['GET'])
def get_code(request, code_id):
    """Get single code file"""
    try:
        code = get_object_or_404(ESP32Code, id=code_id)
        return Response({
            'id': code.id,
            'name': code.name,
            'code': code.code,
            'language': code.language,
            'description': code.description,
            'created_at': code.created_at.isoformat(),
            'updated_at': code.updated_at.isoformat(),
        })
    except Exception as e:
        return Response({"error": str(e)}, status=status.HTTP_400_BAD_REQUEST)


@api_view(['POST'])
def save_code(request):
    """Save code file"""
    try:
        data = request.data
        code_id = data.get('id')
        
        if code_id:
            # Update existing code
            code = get_object_or_404(ESP32Code, id=code_id)
            code.name = data.get('name', code.name)
            code.code = data.get('code', code.code)
            code.language = data.get('language', code.language)
            code.description = data.get('description', code.description)
            code.save()
        else:
            # Create new code
            code = ESP32Code.objects.create(
                name=data.get('name', 'Untitled'),
                code=data.get('code', ''),
                language=data.get('language', 'cpp'),
                description=data.get('description', ''),
            )
        
        return Response({
            'id': code.id,
            'name': code.name,
            'message': 'Code saved successfully'
        }, status=status.HTTP_200_OK)
    except Exception as e:
        return Response({"error": str(e)}, status=status.HTTP_400_BAD_REQUEST)


@api_view(['DELETE'])
def delete_code(request, code_id):
    """Delete code file"""
    try:
        code = get_object_or_404(ESP32Code, id=code_id)
        code.delete()
        return Response({"message": "Code deleted successfully"}, status=status.HTTP_200_OK)
    except Exception as e:
        return Response({"error": str(e)}, status=status.HTTP_400_BAD_REQUEST)
    