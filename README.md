# UV project

### To start the project

---
- Initialise python virtual environment
- ```cd ./djangoapp/ ```
- ```python manage.py runserver```
- Open http://127.0.0.1:8000/

Don't forget to always install ```
pip install```

We are using a uv index endpoint to get the information
https://www.openuv.io \

You need to create a ```.env``` file in [```djangoapp/```](djangoapp)
It has to contain:
```
UV_API_KEY="" #here is the key you get from the open uv
REDIS_URL="redis://localhost:6379/0"
```

You also need a running docker container using image ```redis/redis-stack``` \
Before starting the code start the container
```commandline
docker run -d --rm -p 6379:6379
```

Now we can start workers that every 30 minutes will update readings from api \
Start the worker in [```djangoapp/```](djangoapp)
```commandline
celery -A uvsite worker --loglevel=info
```

and then the beat
```commandline
celety -A uvsite beat --loglevel=info
```