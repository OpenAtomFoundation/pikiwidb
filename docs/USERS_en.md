## 1. Qihoo 360
<img src="http://i.imgur.com/dcHpCm4.png" height = "50" width = "60" alt="Qihoo360">

At 360, Pika has replaced almost all large-capacity Redis instances and all SSDB instances. Currently there are 1000+ instances, with 90 billion accesses per day, 18 TB of storage capacity, equivalent to approximately 54 TB of memory.

## 2. Sina Weibo
<img src="http://i.imgur.com/jjZczkN.png" height = "50" width = "60" alt="Weibo">

Use cases:
1. File storage cluster, using file identifier IDs.
2. Search uses some user attribute features with Pika as one of the storage material libraries.
3. Background spam filtering, used as anti-spam.

In production.

## 3. Garena
<img src="http://i.imgur.com/zoel46r.gif" height = "50" width = "60" alt="Garena">

Use cases:
1. Used for Timeline feature, read/write ratio 4:1, data volume over 100 GB, QPS in the tens of thousands.
2. E-commerce platform recommendation feature.

## 4. Apus
<img src="http://i.imgur.com/kHqACbn.png" height = "50" width = "60" alt="Apus">
In production.

## 5. Ffan E-commerce
<img src="http://i.imgur.com/2c57z8U.png" height = "50" width = "60" alt="Ffan">

Used as offline backup for large-volume online Redis data.

## 6. Meituan
<img src="http://i.imgur.com/rUiO5VU.png" height = "50" width = "60" alt="Meituan">

1. Big data, push services (in production).
2. Using Pika's storage engine nemo to provide multi-data-structure interfaces for internal NoSQL (in testing, preparing for production).

## 7. XES Online School (Xueersi)
<img src="http://i.imgur.com/px5mEuW.png" height = "50" width = "60" alt="XES">

Persistent data storage (in production).

## 8. Easemob (Huanxin)
<img src="http://imgur.com/yJe4FP8.png" height = "50" width = "60" alt="HX">
Used for storing offline data messages in push services.

## 9. Xunlei (Thunder)
<img src="http://i.imgur.com/o8ZDXCH.png" height = "50" width = "60" alt="XL">

Stores personalized recommendation data for users, currently using 15 machines.

In production.

## 10. Gaoweida
<img src="http://imgur.com/w3qNQ9T.png" height = "50" width = "60" alt="GWD"> 

Records mobile device access logs and marks active status.

In production.

## 11. Diyidan
<img src="https://imgur.com/KMVr3Z6.png" height = "50" width = "60" alt="DYD">

In production.

## 12. Yima Technology
<img src="http://i.imgur.com/vJbAfri.png" height = "50" width = "60" alt="YM">

In production.

## 13. Xiaomi
<img src="http://i.imgur.com/aNxzwsY.png" height = "50" width = "60" alt="XM">

In production.

## 14. 58.com
<img src="http://i.imgur.com/mrWxwkF.png" height = "50" width = "60" alt="XL">

In production.

## 15. 360 Games
<img src="http://i.imgur.com/ktPV3JU.jpg?2" height = "50" width = "60" alt="XL">

360 Games has fully completed the migration from SSDB to Pika.

## 15. Cheetah Mobile (Liebao Mobile)
<img src="http://i.imgur.com/DX6Ey4p.jpg" height = "50" width = "60" alt="LB">

Used for storing large amounts of page data and offline user computation data.

## 16. Mingshitang Education
<img src="http://imgur.com/0oaVKlk.png" height = "50" width = "60" alt="MST">

The Venus platform has launched with pika+QConf; other systems are gradually coming online.

## 17. Maimai
<img src="https://imgur.com/qN6z25x.png" height = "50" width = "60" alt="MM">

In production.

## 18. Vipshop
<img src="https://i.imgur.com/G9MOvZe.jpg" height = "50" width = "60" alt="VIP">

In production.

## 19. Traffic Condition Eye (Lukuang Traffic)
<img src="https://imgur.com/vQW5qr3.png" height = "50" width = "60" alt="LK">

In production, storing traffic condition information.

## 20. Douyu TV
<img src="https://s1.ax1x.com/2020/05/02/JjyV5q.th.png" height = "50" width = "60" alt="DY">

In production.

## 21. SeaGroup
<img src="https://s1.ax1x.com/2020/05/19/Y5Mzi6.png" height = "50" width = "60" alt="SEA">

Deployed using codis + pika cluster mode.
