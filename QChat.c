#define FUSE_USE_VERSION 31
#define debug

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

static const int maxNameLength = 10;

static struct qNode{
    char * name;
    char * content;
    int specie;                 // 0 for directory entry ; 1 for normal file
    struct qNode * list_ptr;    // record subfiles
    struct qNode * next;        // for linkedlist
    struct qNode * prev;
};

static struct qNode * root;

/*
    Alloc new Node
*/
static struct qNode* malloc_node(){
    void * ptr = malloc(sizeof(struct qNode));
    memset(ptr,0,sizeof(struct qNode));
    return (struct qNode*) ptr;
}

static void set_node(struct qNode * node, char * _name, char * _content, int _specie, 
                struct qNode * p1, struct qNode * p2, struct qNode * p3){
    node->name = _name;
    node->content = _content;
    node->specie = _specie;
    node->list_ptr = p1; node->next = p2; node->prev = p3;
}

/*
    Avoid memory leak.
*/
static void free_node(struct qNode * node){
    if(node->content != NULL) free(node->content);
    if(node->name != NULL) free(node->name);
    struct qNode * tmpPtr = node->list_ptr;
    while(tmpPtr != NULL){
        struct qNode * tmpTmpPtr = tmpPtr;
        tmpPtr = tmpPtr->next;
        free_node(tmpTmpPtr);
    }
    free(node);
}

/*
    Find specific qNode from the root of the hierachy
*/
static int find_node(struct qNode** result, const struct qNode* _root, const char * path){
    int stringPtr = 1;
    while(path[stringPtr] != '\0'){
        int isFound = 0; // 0 for not found; 1 found;
        int isDir = 0; // 0 for common file; 1 for dir
        int tmp = 0;
        char tmpName[maxNameLength];
        while(path[stringPtr] != '/' && path[stringPtr] != '\0'){
            tmpName[tmp++] = path[stringPtr++];
        }
        tmpName[tmp] = '\0';
        if(path[stringPtr] == '/') isDir = 1;
        struct qNode * nodePtr = _root->list_ptr;
        while(nodePtr != NULL){
            if(strcmp(nodePtr->name,tmpName) == 0){
                if((isDir == 1) && (nodePtr->specie == 1)) return -ENOENT;
                _root = nodePtr;
                isFound = 1;
                break;
            }
            nodePtr = nodePtr->next;
        }
        if(isFound == 0) return -ENOENT;
    }
    (*result) = _root;
    return 0;
}

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
static struct options {
	int show_help;
} options;

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};

static void *qchat_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 1;
	return NULL;
}

static int qchat_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
	(void) fi;
	memset(stbuf, 0, sizeof(struct stat));
    struct qNode * target;
    int res = find_node(&target, root, path);
    if(res != 0) return res;
    if(target->specie == 0){
        stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
    }else{
        stbuf->st_mode = S_IFREG | 0770;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(target->content);
    }
	return 0;
}

static int qchat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;

    struct qNode * target;
    int res = find_node(&target, root, path);
    if(res != 0) return res;

	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
    target = target->list_ptr;
    while(target != NULL){
        filler(buf, target->name, NULL, 0, 0);
        target = target->next;
    }
	return 0;
}

static int qchat_open(const char *path, struct fuse_file_info *fi)
{
    struct qNode * target;
    int res = find_node(&target, root, path);
    if(res != 0) return -ENOENT;
	return 0;
}

static int qchat_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
    struct qNode * target;
    int res = find_node(&target, root, path);
    if(res != 0) return -ENOENT;

	len = strlen(target->content);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, target->content + offset, size);
	} else
		size = 0;

	return size;
}

static const struct fuse_operations qchat_oper = {
    .init       =   qchat_init,
    .getattr    =   qchat_getattr,
    .readdir    =   qchat_readdir,
    .open       =   qchat_open,
    .read       =   qchat_read,
};

static void show_help(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
	       "    --help          get useful information\n"
	       "\n");
}

int main(int argc, char *argv[])
{
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	/* Parse options */
	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0][0] = '\0';
	}

    root = malloc_node();
    set_node(root,NULL,NULL,0,NULL,NULL,NULL);

    #ifdef debug
    struct qNode * tmp = malloc_node();
    set_node(tmp,strdup("srb"),strdup("ZhaoYiLong is srb's father."),1,NULL,NULL,NULL);
    root->list_ptr = tmp;
    #endif

	ret = fuse_main(args.argc, args.argv, &qchat_oper, NULL);
	fuse_opt_free_args(&args);
    free_node(root);
	return ret;
}
