from django.contrib import admin
from .models import ESP32Code

# Register your models here.

@admin.register(ESP32Code)
class ESP32CodeAdmin(admin.ModelAdmin):
    list_display = ['name', 'language', 'created_at', 'updated_at']
    list_filter = ['language', 'created_at']
    search_fields = ['name', 'description', 'code']
    readonly_fields = ['created_at', 'updated_at']
    fieldsets = (
        ('Basic Information', {
            'fields': ('name', 'language', 'description')
        }),
        ('Code Content', {
            'fields': ('code',)
        }),
        ('Timestamps', {
            'fields': ('created_at', 'updated_at'),
            'classes': ('collapse',)
        }),
    )
