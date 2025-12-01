import json
import math
from django.http import HttpResponse, JsonResponse
from django.shortcuts import render
from django.views.decorators.http import require_GET
from django.views.decorators.csrf import csrf_exempt
from .models import UVData

# ---------------------------------------------------------
# EXISTING VIEWS
# ---------------------------------------------------------
def index(request):
    return HttpResponse("This is dashboard")

def uv_page(request):
    return render(request, "dashboard/uv_display.html")

@require_GET
def api_uv_current(request):
    # Get the NEWEST reading
    latest = UVData.objects.order_by('-timestamp').first()
    if latest:
        return JsonResponse({'uv': latest.uv_value, 'timestamp': latest.timestamp.isoformat()})
    else:
        return JsonResponse({'error': 'No UV data found'})

@require_GET
def api_uv_history(request):
    readings = UVData.objects.all()[:48]
    data = [{'uv': r.uv_value, 'timestamp': r.timestamp.isoformat()} for r in readings]
    return JsonResponse(data, safe=False)

# ---------------------------------------------------------
# SENSOR LOGIC & MATH
# ---------------------------------------------------------

def calculate_ita(readings):
    if not readings: return 0
    avg = sum(readings) / len(readings)
    # Mapping 2.0 -> 5 and 12.0 -> 85
    ita = (avg - 2.0) * 8.0 + 5.0
    return round(ita, 1)

def determine_skin_type(readings):
    if not readings: return "Unknown"
    avg_reflection = sum(readings) / len(readings)
    
    if avg_reflection > 11.0: return "Type I (Pale White)"
    elif avg_reflection > 9.5: return "Type II (White)"
    elif avg_reflection > 7.5: return "Type III (Cream White)"
    elif avg_reflection > 6.0: return "Type IV (Moderate Brown)"
    elif avg_reflection > 4.0: return "Type V (Dark Brown)"
    else: return "Type VI (Deeply Pigmented)"

def get_detailed_recommendation(skin_type, uv_index):
    # Default values
    rec = {
        "spf": "None required", 
        "time": "N/A", 
        "warning": None 
    }

    # --- EXTREME / VERY HIGH WARNING (10+) ---
    # CHANGED: Threshold lowered from 11 to 10
    if uv_index >= 10:
        if "Type I (" in skin_type or "Type II (" in skin_type:
            rec["spf"] = "SPF 50+ (CRITICAL)"
            rec["time"] = "Every 60 mins"
            rec["warning"] = "⚠️ DANGER (UV 10+): Unprotected skin will burn in minutes. STAY INDOORS."
        
        elif "Type III (" in skin_type or "Type IV (" in skin_type:
            rec["spf"] = "SPF 50+"
            rec["time"] = "Every 90 mins"
            rec["warning"] = "⚠️ Extreme UV (10+): Avoid sun between 10AM-4PM. Shirt & Hat essential."
            
        else: # Type V/VI
            rec["spf"] = "SPF 30-50"
            rec["time"] = "Every 2 hours"
            rec["warning"] = "⚠️ Extreme UV (10+): Seek shade. Protection required."

    # --- High UV (7 - 9.9) ---
    elif uv_index >= 7:
        if "Type I (" in skin_type or "Type II (" in skin_type:
            rec["spf"] = "SPF 50+"
            rec["time"] = "Every 2 hours"
        elif "Type III (" in skin_type:
            rec["spf"] = "SPF 30-50"
            rec["time"] = "Every 2 hours"
        elif "Type IV (" in skin_type:
            rec["spf"] = "SPF 30"
            rec["time"] = "Every 3 hours"
        else: # Type V/VI
            rec["spf"] = "SPF 30"
            rec["time"] = "Every 4 hours"

    # --- Moderate UV (3 - 6.9) ---
    elif uv_index >= 3:
        if "Type I (" in skin_type or "Type II (" in skin_type:
            rec["spf"] = "SPF 30"
            rec["time"] = "Every 2 hours"
        elif "Type III (" in skin_type:
            rec["spf"] = "SPF 30"
            rec["time"] = "Every 3 hours"
        elif "Type IV (" in skin_type:
            rec["spf"] = "SPF 15-30"
            rec["time"] = "Every 3 hours"
        else: # Type V/VI
            rec["spf"] = "SPF 15"
            rec["time"] = "Every 4 hours"

    # --- Low UV (<3) ---
    else:
        rec = {"spf": "None required", "time": "N/A (Low UV)", "warning": None}
        
    return rec

@csrf_exempt 
def calculate_spf(request):
    if request.method == 'POST':
        try:
            data = json.loads(request.body)
            readings = data.get('readings', [])
            
            latest_uv = UVData.objects.order_by('-timestamp').first()
            current_uv_index = latest_uv.uv_value if latest_uv else 0
            
            skin_type = determine_skin_type(readings)
            ita_score = calculate_ita(readings)
            advice = get_detailed_recommendation(skin_type, current_uv_index)
            
            return JsonResponse({
                'status': 'success',
                'skin_type': skin_type,
                'spf_recommendation': advice['spf'],
                'reapply_time': advice['time'], 
                'warning': advice['warning'],
                'uv_index': current_uv_index,
                'ita_score': ita_score
            })
        except Exception as e:
            return JsonResponse({'status': 'error', 'message': str(e)})

    return JsonResponse({'status': 'error', 'message': 'Only POST allowed'})