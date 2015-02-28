/**************************************************************************\
| Toledo Nanochess (c) Copyright 2010 Oscar Toledo G. All rights reserved  |
| 1257 non-blank characters.                 biyubi@gmail.com  Jan/11/2010 |
| o This version includes a minimal Winboard adapter (699 extra bytes)     |
| o Full legal chess moves.                     http://nanochess.110mb.com |
| o Remove these comments to get 2025 bytes source code (*NIX end-of-line) |
\**************************************************************************/
#include <stdio.h>
#include <time.h>
char*l="ustvrtsuqqqqqqqqyyyyyyyy}{|~z|{}"
"   76Lsabcddcba .pknbrqmove %s\n\0?A6J57IKJT576,+-48HLSU";
#define v X(0,0,0,21,
#define Z while(
#define _ ;if(
#define P return--G,y^=8,
B,i,y,u,b,I[411],*G=I,x=10,z=15,M=1e4,f,j,m,n,t,o,L,E,D,O=100;X(w,c,h,e,S,s){
int t,o,L,E,d,O=e,N=-M*M,K=78-h<<x,p,*g,n,*m,A,q,r,C,J,a=y?-x:x;D++;y^=8;G++;d=
w||s&&s>=h&&v 0,0)>M;do{_ o=I[p=O]){q=o&z^y _ q<7){A=q--&2?8:4;C=o-9&z?q[
"& .$  "]:42;do{r=I[p+=C[l]-64]_!w|p==w){g=q|p+a-S?0:I+S _!r&(q|A<3||g)||(r+1&z
^y)>9&&q|A>2){_ m=!(r-2&7))P G[1]=O,K;J=n=o&z;E=I[p-a]&z;t=q|E-7?n:(n+=2,6^y);Z
n<=t){L=r?l[r&7]*9-189-h-q:0 _ s)L+=(1-q?l[p/x+5]-l[O/x+5]+l[p%x+6]*-~!q-l[O%x+
6]+o/16*8:!!m*9)+(q?0:!(I[p-1]^n)+!(I[p+1]^n)+l[n&7]*9-386+!!g*99+(A<2))+!(E^y^
9)_ s>h||1<s&s==h&&L>z|d){p[I]=n,O[I]=m?*g=*m,*m=0:g?*g=0:0;L-=X(s>h|d?0:p,L-N,
h+1,G[1],J=q|A>1?0:p,s)_!(h||s-1|B-O|i-n|p-b|L<-M))P y^=8,u=J;J=q-1|A<7||m||!s|
d|r|o<z||v 0,0)>M;O[I]=o;p[I]=r;m?*m=*g,*g=0:g?*g=9^y:0;}_ L>N){*G=O _ s>1){_ h
&&c-L<0)P L _!h)i=n,B=O,b=p;}N=L;}n+=J||(g=I+p,m=p<O?g-3:g+2,*m<z|m[O-p]||I[p+=
p-O]);}}}}Z!r&q>2||(p=O,q|A>2|o>z&!r&&++C*--A));}}}Z++O>98?O=20:e-O);P N+M*M&&N
>-K+1924|d?N:0;}
#define D(z)_!strcmp(g,#z))
main(){char*a,g[80];clock_t k;setbuf(stdout,0);Z fgets(g+x,69,stdin)){sscanf(g+
x,"%9s%d%d",g,&n,&D)D(quit)break D(force)f=1 D(post)j=1 D(nopost)j=0 D(level)o=
n,E=n?n:20,O=D*6e3/E D(time)O=n/(E-L%E+1)D(new){_*l-'u')l-=32;G=I;B=y=u=f=L=0;Z
++B<121)*G++=B/x%x<2|B%x<2?7:B/x&4?0:*l++&31;}D(go)f=0 D(protover)puts(
"feature myname=\"Toledo Nanochess Jan/11/2010\" done=1")_ isalpha(*g)&&isdigit
(g[1])){a=g;B=*a++&z;B+=100-(*a++&z)*x;b=*a++&z;b+=100-(*a++&z)*x;i=(*a-'q'?*a-
'r'?*a-98?*a-'n'?I[B]&z^y:11:12:13:14)^y;v u,1)_!f)strcpy(g,"go");}D(go){k=
clock();D=0;n=O<50?4:5;B=21;do{m=X(0,0,0,B,u,n);t=(clock()-k)*1e2/
CLOCKS_PER_SEC;*g=B%x+96;g[1]=58-B/x;g[2]=b%x+96;g[3]=58-b/x;g[4]=I[B]-i&z?l[i&
7|16]:0;g[5]=0;n++;_ j)printf("%d %d %d %d %s\n",n,m,t,D,g);}Z m>-M&m<M&t*x<O);
_ o)L++;v u,1);printf(l+23,g);}}}
