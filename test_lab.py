import socket, subprocess, time, signal

def start():
    p = subprocess.Popen(['./server'])
    time.sleep(.15)
    s = socket.create_connection(('127.0.0.1',9001)); s.settimeout(2)
    return p,s

def request(s, command):
    s.sendall(command.encode())
    data=b''
    while b'\0' not in data:
        part=s.recv(1024)
        assert part, 'Unexpected disconnect'
        data+=part
    return data.split(b'\0')[0].decode()

p,s=start()
cases=[('print','NULL'),('get_length','Length = 0'),('add_back 5','ACK5'),('add_front 2','ACK2'),('add_position 1 3','ACK3'),('add_position 4 -7','ACK-7'),('print','3->2->5->-7->NULL'),('get 2','VALUE = 2'),('get_length','Length = 4'),('remove_position 2','ACK2'),('remove_back','ACK-7'),('remove_front','ACK3'),('remove_back','ACK5'),('remove_front','ACK-1'),('print','NULL'),('get 0','VALUE = -1')]
for command,expected in cases:
    actual=request(s,command)
    assert actual==expected,(command,expected,actual)
for command in ['add_front','add_back abc','get','add_position 0 9','unknown']:
    assert request(s,command).startswith('ERROR'),command
s.sendall(b'exit'); s.close(); assert p.wait(timeout=2)==0
print('PASS: all operations, boundaries, malformed input, exit')
p,s=start(); request(s,'add_back 10'); s.close(); assert p.wait(timeout=2)==0
print('PASS: disconnect cleanup')
p,s=start(); request(s,'add_back 10'); p.send_signal(signal.SIGINT); assert p.wait(timeout=2)==0; s.close()
p=subprocess.Popen(['./server']); time.sleep(.15); p.send_signal(signal.SIGINT); assert p.wait(timeout=2)==0
print('PASS: Ctrl-C while connected and while accepting')
p=subprocess.Popen(['./server']); time.sleep(.15)
r=subprocess.run(['./client'],input='menu\nadd_back 42\nprint\nget 1\nexit\n',text=True,capture_output=True,timeout=3)
assert r.returncode==0,r.stderr
assert '42->NULL' in r.stdout and 'VALUE = 42' in r.stdout,r.stdout
assert p.wait(timeout=2)==0
print('PASS: interactive client end-to-end')
p=subprocess.Popen(['./server']); time.sleep(.15)
commands=''.join('add_back %d\n' % i for i in range(500))+'print\nexit\n'
r=subprocess.run(['./client'],input=commands,text=True,capture_output=True,timeout=10)
expected=''.join('%d->' % i for i in range(500))+'NULL'
assert r.returncode==0 and expected in r.stdout
assert p.wait(timeout=2)==0
print('PASS: 500-node list output exceeds 1024 bytes without truncation')
