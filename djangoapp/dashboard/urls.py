from django.urls import path
from . import views

urlpatterns = [
    path('', views.uv_page, name='dashboard_index'),
    path("api/uv/", views.api_uv_current, name="api_uv_current"),
    path("api/uv/history/", views.api_uv_history, name="api_uv_history"),
]