from django.urls import path
from . import views

urlpatterns = [
    path('uv/', views.receive_uv_data, name='receive_uv_data'),
    path('editor/', views.code_editor, name='code_editor'),
    path('api/codes/', views.list_codes, name='list_codes'),
    path('api/codes/<int:code_id>/', views.get_code, name='get_code'),
    path('api/codes/save/', views.save_code, name='save_code'),
    path('api/codes/<int:code_id>/delete/', views.delete_code, name='delete_code'),
]
