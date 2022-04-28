all: 
	gcc -Wall QChat.c `pkg-config fuse3 --cflags --libs` -o QChat
run:
	mkdir QChatRoom
	./QChat QChatRoom

clean:
	umount QChatRoom
	rm Qchat
	rm -rf QChatRoom
test:
	mkdir QChatRoom/Alice QChatRoom/Bob QChatRoom/Cleo QChatRoom/Alice/Friends QChatRoom/Bob/Sons QChatRoom/Bob/Baby
	touch QChatRoom/Alice/Friends/Bob QChatRoom/Bob/Sons/Alice QChatRoom/Alice/Cleo QChatRoom/Bob/Baby/Cleo QChatRoom/Cleo/Bob QChatRoom/Cleo/Alice