include $(top_srcdir)/common/common.am

bin_PROGRAMS = modbus_server client

modbus_server_SOURCES = modbus_server.c
modbus_server_CFLAGS = $(AM_CFLAGS) $(MODBUS_CFLAGS)
modbus_server_LDADD = $(AM_LIBS) $(MODBUS_LIBS) \
			$(top_srcdir)/common/libcommon.la

client_SOURCES = client.c
client_CFLAGS = $(AM_CFLAGS) $(MODBUS_CFLAGS)
client_LDADD = $(AM_LIBS) $(MODBUS_LIBS)
