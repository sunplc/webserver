#!/bin/bash

# apr_socket_recv这个是操作系统内核的一个参数，在高并发的情况下，内核会认为系统受到了SYN flood攻击，会发送cookies（possible SYN flooding on port 80. Sending cookies），这样会减慢影响请求的速度，所以在应用服务武器上设置下这个参数为0禁用系统保护就可以进行大并发测试了：

# vim /etc/sysctl.conf 
# net.ipv4.tcp_syncookies = 0
# sysctl -p
# 然后就可以超过1000个并发测试了。
 
# 另附其他系统内核参数说明：
 
# net.ipv4.tcp_syncookies = 0  
#此参数是为了防止洪水攻击的，但对于大并发系统，要禁用此设置
 
# net.ipv4.tcp_max_syn_backlog
#参数决定了SYN_RECV状态队列的数量，一般默认值为512或者1024，即超过这个数量，系统将不再接受新的TCP连接请求，一定程度上可以防止系统资源耗尽。可根据情况增加该值以接受更多的连接请求。
 
# net.ipv4.tcp_tw_recycle
#参数决定是否加速TIME_WAIT的sockets的回收，默认为0。
 
# net.ipv4.tcp_tw_reuse
#参数决定是否可将TIME_WAIT状态的sockets用于新的TCP连接，默认为0。
 
# net.ipv4.tcp_max_tw_buckets
#参数决定TIME_WAIT状态的sockets总数量，可根据连接数和系统资源需要进行设置。 


# 并发过高会被系统限制
# ab参数： -n 总数，-c 并发数，-t 超时时间，-k KeepAlive 复用连接
ab -n 30000 -c 500 http://127.0.0.1:8080/

