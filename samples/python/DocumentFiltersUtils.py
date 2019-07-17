import sys


def ToUnicodeString(py_string):
	if sys.version_info[0] >= 3:
		if isinstance(py_string, str):
			return py_string
	else:
		if isinstance(py_string, unicode):
			return py_string
	return py_string.decode('utf8')


def OutputStringToConsole(out_string):
	if sys.version_info[0] >= 3:
		sys.stdout.buffer.write(out_string.encode('utf8', 'ignore'))
	else:
		sys.stdout.write(out_string.encode('utf8', 'ignore'))
