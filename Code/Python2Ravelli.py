import urllib.request
root_url = "http://192.168.1.200"  # ESP's IP

def get_data(url):
	global data
	n = urllib.request.urlopen(url).read()
	n = n.decode("utf-8")
	data = n

def main():
	get_data(root_url+"/FanPower-0")
	print(data)
	MonTest = False

main()