from django.db import models

# Create your models here.
class UVData(models.Model):
    uv_value = models.FloatField()
    timestamp = models.DateTimeField(auto_now_add=True)

    class Meta:
        ordering = ['-timestamp']

    def __str__(self):
        return str(self.uv_value)