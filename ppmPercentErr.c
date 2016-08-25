#include <stdio.h>
#include <stdlib.h>

typedef struct {
	int x, y;
	char p;
} point;

int debug = 1, w, h, maxC;
double err;
int parseInt(char *str, int indx);
inline static void saveImg(char *pix, int n, char *fName, int nLen);
inline int comparePoints(point *a, point *b);
inline void generatePoint(point *a);

void main(int argc, char *argv[]) {//Possitive (> 0) return values indicate programatic error, negative (< 0) value indicate user error, 0 indicates no errors
	if(argc < 6 && !debug) {
		printf("Incorect Use: %s <Img> <% Error> <% Error Magnitude> <Itterations> <Output at each step: y/n> <Optional: Output File Name Base>\n", argv[0]);
		printf("Note: % Error and Itterations must be less than double maxval and are formatted as ints for input (no check in place, risk of stack overflow)\n");
		printf("	% Error Magnitude is the precision (Ex: 1234 -2 == 12.34%), % Error capped at 100%\n	Output setting must be lowercase\n");
		return;
	}
	
	err = (double) parseInt(argv[2], 0);
	{
		int m = parseInt(argv[3], 0);
		if(m > 0) while(m) {m--; err *= 10;}
		else if(m < 0) while(m) {m++; err /= 10;}
		err /= 100;
	}
	
	int Ittrs = parseInt(argv[4], 0);
	
	if(Ittrs < 0) {
		printf("Itterations MUST be positive (> 0)! \n");
		return;
	}
	
	char output = argv[5][0];
	
	if(output != 'y' && output != 'n') {
		printf("<Output At Each Step> must be lowercase y or n\n");
		return;
	}
	
	int nl = 0;
	{
		char *t;
		if(argc == 7) t = argv[6];
		else t = argv[ 1];
		
		while(t) { nl++; t++; }
	}
	
	FILE *f = fopen(argv[1], "rb");
	if(!f) { printf("Error: Unable to open file %s!!!\n", argv[1]); fclose(f); return; }
	char magNum[3];//"magic number" 1st number in file
	fgets(magNum, 3, f);
	if(magNum[0] != 'P' || magNum[1] != '6') {printf("Error File %s does not have magic number 'P6'! Magic Number == '%s'! Exiting!!!\n", argv[1], magNum); fclose(f); return; }
	
	{
		char buf[16];
		fgets(buf, 16, f);
		fgets(buf, 16, f);
		w = parseInt(buf, 0);//width 2nd number
		fgets(buf, 16, f);
		h = parseInt(buf, 0);//height 3rd number
		fgets(buf, 16, f);
		maxC = parseInt(buf, 0);//max color value 4th number
	}//it is a bit stupid that I am doing this, but this should help to optimize memory usage a little
	int c = 'a';//arbitrary value just incase c happens to be EOF when defined
	
	char pic[h][w][3];//h w for optimization -- h is an unknow distance from h - 1 and h + 1 while w is almost always right next to w - 1 and w + 1, 3 because RGB value
	{
		int i, j, x;
		for(i = 0; i < h && c != EOF; i++) {
			for(j = 0; j < w && c != EOF; j++) for(x = 0; x < 3 && (c = fgetc(f)) != EOF; x++) pic[i][j][x] = (char) c;
			fgetc(f); c = fgetc(f);
		}
		
		srand(((((((int) pic[0][0][0]) << 8) + (int) pic[0][0][1]) << 8) + (int) pic[0][0][2]) ^ rand());//generate somewhat, but not really random seed for rng. First bit builds part of an int from 3 chars (4 needed for entire int), second bit makes it a tad bit more random by XORing it by the next random number
		
		if(debug) {
			printf("%s <%d, %d> %d\n", magNum, w, h, maxC);
			for(i = 0; i < h; i++) {
				for(j = 0; j < w; j++) {
					for(x = 0; x < 3; x++) printf("%c", pic[i][j][x]);
					printf(" ");
				}
				printf("\n");
			}
		}
	}
	
	if(ferror(f)) { printf("I/O Error reading File %s! Now Exiting!!!\n", argv[1]); fclose(f); return; }
	else if(feof(f)) printf("File %s Succesfully read!\n", argv[1]);
	else printf("Not all of %s read, continuing anyways.\n", argv[1]);
	
	fclose(f);
	
	unsigned long int maxCorrupt = h * w * 3;
	unsigned long int clen;
	point *corps, *pnt = malloc(sizeof(point));
	if(err > 1) { corps = malloc(sizeof(point) * maxCorrupt); clen = maxCorrupt; }
	else { corps = malloc(sizeof(point) * (int) ((double) maxCorrupt * err)); clen = (double) maxCorrupt * err; }
	
	unsigned long int i, j, IT = 0;
	char exist, nw, rn;
	while(IT < Ittrs) {
		IT;
		i = 0;
		while(i < clen) {
			nw = 1;
			while(nw) {
				generatePoint(pnt);
				exist = 1;
				j = 0;
				while(exist && j < i) {
					if(comparePoints(pnt, corps + j)) exist = 0;
					j++;
				}
				if(exist) {
					nw = 0;
					corps[i] = *pnt;
				}
			}
			rn = (char) rand();
			while(!rn) rn = (char) rand();//garentees number is (> 0)
			pic[(*pnt).x][(*pnt).y][(*pnt).p] ^= rn;//XOR pixel with random number
			i++;
		}
		if(output == 'y') saveImg((char *)pic, IT, argc == 7 ? argv[6] : argv[1], nl);
	}
	if(output == 'n') saveImg((char *)pic, 0, argc == 7 ? argv[6] : argv[1], nl);
	
	free(corps);
	free(pnt);
}


inline static void saveImg(char *pixPtr, int n, char *fName, int nLen) {
	//char *(pix[w][h][3])=NULL;
	//)=(char *)pixPtr;//h w for optimization -- h is an unknow distance from h - 1 and h + 1 while w is almost always right next to w - 1 and w + 1, 3 because RGB value

	int NDIG = 0, pow = 1;
	while(n >= pow) {pow *= 10; NDIG++;}
	if(!NDIG) NDIG = 1;
	char oName[nLen + NDIG + 5];//add 5 because char count for ".ppm" is 5 including NULL terminator
	for(int I = 0; I < nLen; I++) oName[I] = fName[I];
	int temN = n;
	for(int I = 0; I < NDIG; I++) {
		oName[nLen + I] = (temN % 10) + '0';
		temN /= 10;
	}
	oName[nLen + NDIG] = '.'; oName[nLen + NDIG + 1] = 'p'; oName[nLen + NDIG + 2] = 'p'; oName[nLen + NDIG + 3] = 'm'; oName[nLen + NDIG + 4] = '\0';//adds file extension and NULL terminator for the string
	
	FILE *f2 = fopen(oName, "wb");
	fputs("P6\n", f2);
	fprintf(f2, "%d\n%d\n%d\n", w, h, maxC); 
	for(int I = 0; I < h; I++) {
		//for(int J = 0; J < w; J++) fwrite(pix[I][J], 1, 3, f2);
		fwrite()
		fputs("\n", f2);
	}
	fclose(f2);
}

inline void generatePoint(point *a) {
	(*a).x = rand() % h;
	(*a).y = rand() % w;
	(*a).p = rand() & 0b10;
	if(!(*a).p) (*a).p = rand() & 0b1;//cant use 3 because that would be index out of bounds (which isnt caught in c... it just keeps reading)
	return;
}

inline int comparePoints(point *a, point *b) {
	return ((*a).x == (*b).x && (*a).y == (*b).y && (*a).p == (*b).p) ? 0 : 1;
} //if points are same: return false, else return true //Done this way as if the same then new one needs to be generated

int parseInt(char *str, int indx) {
	int r = 0;
	char *s = str + indx;
	char inv = 0;
	if(*s == '-') {inv = 1; s++;}
	while(*s >= '0' && *s <= '9') {
		r *= 10;
		r += *s - '0';
		s++;
	}
	if(*s != 0) {
		printf("Numeric Input must contain only numbers or may start with the character '-'\nWarning: Unable to stop program! Continueing with current values!\n");
	}
	if(inv) r *= -1;
	return r;
}
