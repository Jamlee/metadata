
### ʵ��ԭ��

��ȡ hostname

````
curl http://localhost:8000/latest/local-hostname
````

��ȡ����

````
wget -q -O - --header "DomU_Request: send_my_password" "192.168.4.151:8080"
wget -t 3 -T 20 --header "DomU_Request: saved_password" "${SERVER_IP}":8080
````