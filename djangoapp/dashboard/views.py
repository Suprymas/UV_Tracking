from django.http import HttpResponse
from django.shortcuts import render
from django.http import JsonResponse
from django.utils import timezone
from django.views.decorators.http import require_GET
from .models import UVData

def index(request):
    return HttpResponse("This is dashboard")

def uv_page(request):
    # render the HTML template we just created
    return render(request, "dashboard/uv_display.html")

@require_GET
def api_uv_current(request):

    latest = UVData.objects.first()

    if latest:
        return JsonResponse({
            'uv': latest.uv_value,
            'timestamp': latest.timestamp.isoformat(),
        })
    else:
        return JsonResponse({
            'error': 'No UV data found',
        })


@require_GET
def api_uv_history(request):
    readings = UVData.objects.all()[:48]

    data = [{
        'uv': reading.uv_value,
        'timestamp': reading.timestamp.isoformat()
    } for reading in readings]

    return JsonResponse(data, safe=False)
