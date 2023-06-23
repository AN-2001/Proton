cc := gcc
flags := -I/usr/include/SDL2/ -I. -I./core -g #-fsanitize=address
libs := -lm -lSDL2 -lSDL2_ttf 
obj := core/proton.o core/renderer.o core/feel.o \
	   curves.o menu.o filePicker.o  inputPrompt.o knotEditor.o
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
