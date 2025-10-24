from django.http import HttpResponse
from django.shortcuts import render
from django.http import JsonResponse
from django.utils import timezone
from django.views.decorators.http import require_GET

def index(request):
    return HttpResponse("This is dashboard")

def uv_page(request):
    # render the HTML template we just created
    return render(request, "dashboard/uv_display.html")

@require_GET
def api_uv_current(request):
    latest = {
        "uv": 6.7,
        "timestamp": timezone.now().isoformat()
    }
    return JsonResponse(latest)

@require_GET
def api_uv_history(request):
    now = timezone.now()
    history = []
    for i in range(24, 0, -1):
        t = now - timezone.timedelta(hours=i)
        history.append({
            "timestamp": t.isoformat(),
            "uv": max(0, (i % 12) + (i % 3) * 0.2)
        })
    return JsonResponse(history, safe=False)

