ARCH ?= $(shell cat /proc/sys/kernel/arch)
BUILD ?= build
CFLAGS ?= -Wall -Wextra -g -O0
CPPFLAGS ?= -g -O0 -D$(ARCH)=1
OBJECTS := i2cdev.o  main.o pca9685.o pwm_mock.o servo.o status.o


all: pca_test

info:
	echo "ARCH=$(ARCH)"

-include *.d

%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $@.d -o $@ -c $<

pca_test : $(OBJECTS)
	$(CXX) $^ -o $@ -g $(LDFLAGS)


.PHONY: clean transfer debug
debug: pca_test
	gdb ./pca_test

clean:
	rm -f *.o *.d pca_test

transfer: clean
	rsync -avzP . dogo@192.168.1.221:~/i2c-servo	

# scp * dogo@192.168.1.221:~/i2c-servo


