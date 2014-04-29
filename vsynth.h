//define a void function() return type.
typedef void (*pt2Funct)(); // needed for set_hbi_hook
extern unsigned char duty;
extern void synth_hook();
extern pt2Funct setsynth();
extern void synthesize(unsigned char, unsigned int);
