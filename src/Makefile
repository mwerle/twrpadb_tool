TARGET=twrpadb_tool

CFLAGS=-g -O2 -I.
CXXFLAGS=-g -O2 -I.
LDFLAGS=-g

SRCS := main.cpp twrpadbfile.cpp crc32.c md5.c
OBJS=$(subst .cpp,.o,$(SRCS))

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) $(TARGET)


depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ -MF ./.depend;

include ./.depend
