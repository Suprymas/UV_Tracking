from django.db import models
from django.utils import timezone

# Create your models here.

class ESP32Code(models.Model):
    """Store ESP32 code files"""
    name = models.CharField(max_length=200, help_text="Code file name")
    code = models.TextField(help_text="Code content")
    language = models.CharField(
        max_length=50, 
        default='cpp',
        choices=[('cpp', 'Arduino/C++'), ('c', 'C'), ('python', 'MicroPython')],
        help_text="Programming language"
    )
    description = models.TextField(blank=True, help_text="Code description")
    created_at = models.DateTimeField(default=timezone.now)
    updated_at = models.DateTimeField(auto_now=True)
    
    class Meta:
        ordering = ['-updated_at']
        verbose_name = "ESP32 Code"
        verbose_name_plural = "ESP32 Codes"
    
    def __str__(self):
        return self.name
