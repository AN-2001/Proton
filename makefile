cc := gcc
flags := -I/usr/include/SDL2/ -Ofast
libs := -lm -lSDL2 -lSDL2_ttf 
obj := proton.o renderer.o feel.o curves.o menu.o filePicker.o 
proj := proton

all: $(proj)


$(proj): $(obj)
	$(cc) $(flags) -o $@ $(obj) $(libs)

%.o: %.c %.h
	$(cc) $(flags) -c -o $@ $< 

%.o: %.c
	$(cc) $(flags) -c -o $@ $< 

clean:
	rm $(obj) $(proj)
