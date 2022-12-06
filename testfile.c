#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

// RUN formatfs before conducting this test!
char poetry[]="Do not go gentle into that good night,\n"
  "Old age should burn and rave at close of day;\n"
  "Rage, rage against the dying of the light.\n"
  ""
  "Though wise men at their end know dark is right,\n"
  "Because their words had forked no lightning they\n"
  "Do not go gentle into that good night.\n"
  ""
  "Good men, the last wave by, crying how bright\n"
  "Their frail deeds might have danced in a green bay,\n"
  "Rage, rage against the dying of the light.\n"
  ""
  "Wild men who caught and sang the sun in flight,\n"
  "And learn, too late, they grieved it on its way,\n"
  "Do not go gentle into that good night.\n"
  ""
  "Grave men, near death, who see with blinding sight\n"
  "Blind eyes could blaze like meteors and be gay,\n"
  "Rage, rage against the dying of the light.\n"
  ""
  "And you, my father, there on the sad height,\n"
  "Curse, bless, me now with your fierce tears, I pray.\n"
  "Do not go gentle into that good night.\n"
  "Rage, rage against the dying of the light.\n";

int main(int argc, char *argv[]) {
  int ret;
  File f;
  char buf[4096*14];

  // should succeed
  f=create_file("simple");
  printf("ret from create_file(\"blarg\") = %p\n",
	 f);
  fs_print_error();

  if (f) {
    // should succeed
    // ret=write_file(f, "hello", strlen("hello"));
    // printf("ret from write_file(f, \"hello\", strlen(\"hello\") = %d\n",
	//    ret);
    // fs_print_error();

    char buf[SOFTWARE_DISK_BLOCK_SIZE];
    memcpy(buf, poetry, strlen(poetry));

    printf("Poerty len: %d\n",strlen(poetry));
    ret=write_file(f, buf, strlen(poetry));
    ret=write_file(f, buf, strlen(poetry));
    ret=write_file(f, buf, strlen(poetry));
    ret=write_file(f, buf, strlen(poetry));
    ret=write_file(f, buf, strlen(poetry));
    // ret=write_file(f, "hello", strlen("hello"));
    fs_print_error();
    
    // // should succeed
    // printf("Seeking to beginning of file.\n");
    // seek_file(f, 0);
    // fs_print_error();
    
    // should succeed
    bzero(buf, 4000);
    ret=read_file(f, buf,4000);
    printf("ret from read_file(f, buf, strlen(\"hello\") = %d\n",
	   ret);
    printf("buf=\"%s\"\n", buf);
    // printf("%s\n", strlen(buf));
    fs_print_error();
    
    // // should succeed
    // close_file(f);
    // printf("Executed close_file(f).\n");
    // fs_print_error();
  }
  else {
    printf("FAIL.  Was formatfs run before this test?\n");
  }
  
  return 0;
}

  
  
  
  
