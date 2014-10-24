#ifndef DEBUG_H
#define DEBUG_H 1
#if !defined(__PRETTY_FUNCTION__)
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif
#define ERR(format,...) do { \
	fprintf(stderr,"%s:%d %s ERROR:",__FILE__,__LINE__,__PRETTY_FUNCTION__); \
	fprintf(stderr,format,##__VA_ARGS__); \
	fputc('\n',stderr); \
} while(0)

#define WARN(format,...) do { \
	fprintf(stderr,"%s:%d %s WARNING:",__FILE__,__LINE__,__PRETTY_FUNCTION__); \
	fprintf(stderr,format,##__VA_ARGS__); \
	fputc('\n',stderr); \
} while(0)

#define TRACE(format,...) do { \
	fprintf(stderr,"%s:%d %s :",__FILE__,__LINE__,__PRETTY_FUNCTION__); \
	fprintf(stderr,format,##__VA_ARGS__); \
	fputc('\n',stderr); \
} while(0)

#endif