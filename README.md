
### 实现原理

从Cloudstack MetaData中获取 hostname

````
curl http://localhost:8000/latest/local-hostname
````


从Cloudstack MetaData中获取密码

````
wget -q -O - --header "DomU_Request: send_my_password" "192.168.4.151:8080"
wget -t 3 -T 20 --header "DomU_Request: saved_password" "${SERVER_IP}":8080
````