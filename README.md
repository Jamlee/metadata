### Metadata Client

[![Build status](https://ci.appveyor.com/api/projects/status/60mrqfrsuepwmmf3?svg=true)](https://ci.appveyor.com/project/Jamlee/metadata)

Metadata Client 是用于作为agent修改 **Administrator 密码**与**主机名**的工具，需要配合 cloudstack 使用。在 windows 模板中，将 metadata 作为开机启动程序即可。

---

获取 hostname

````
curl http://localhost:8000/latest/local-hostname
````


获取 administrator 密码

````
wget -q -O - --header "DomU_Request: send_my_password" "192.168.4.151:8080"
wget -t 3 -T 20 --header "DomU_Request: saved_password" "${SERVER_IP}":8080
````
