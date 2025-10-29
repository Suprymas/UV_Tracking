from django.urls import path
from . import views

urlpatterns = [
    path('uv/', views.receive_uv_data, name='receive_uv_data'),
]
