FROM python:latest

COPY /output     /app
COPY /assets     /app/
COPY /index.html /app/

EXPOSE 8080

CMD cd /app \
&& python -m http.server 8080
