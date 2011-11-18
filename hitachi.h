/******** プロトタイプ (plib.lib)	11/17/2000  by S.Heike *******************/

typedef struct		/* Header for Hitachi-AFM */
	{
	unsigned short	id;		/* [000] データ識別子(0100h)	data offset	*/
	short				ai;		/*[002] I 現在値 current preset	*/
	short				af;		/*[004] F 現在値 force					*/
	short				am;		/*[006] M 現在値								*/
	short				aa;		/*[008] A 現在値								*/
	short				dx;		/*[010] X 現在値 offset?				*/
	short				dy;		/*[012] Y 現在値								*/
	short				dz;		/*[014] Z 現在値								*/
	short				dv;		/*[016] V 現在値								*/
	short				dxc;	/*[018] Xc現在値								*/
	short				dyc;	/*[020] Yc現在値								*/
	short				dzc;	/*[022] Zc現在値								*/
	short				db;		/*[024] B 現在値								*/
	short				si;		/*[026] I 設定値 init						*/
	short				sf;		/*[028] F 設定値								*/
	short				sm;		/*[030] M 設定値								*/
	short				sa;		/*[032] A 設定値								*/
	double			ri;		/*[034] I 分解能[A]	resolution		*/
	double			rf;		/*[042] F 分解能[N]								*/
	double			rm;		/*[050] M 分解能[Hz]							*/
	double			ra;		/*[058] A 分解能[-]								*/
	double			rx;		/*[066] X 分解能[m]	scaling factor */
	double			ry;		/*[074] Y 分解能[m]								*/
	double			rz;		/*[082] Z 分解能[m]								*/
	double			rv;		/*[090] V 分解能[V]								*/
	double			rxc;	/*[098] Xc分解能[m]	unit in m			*/
	double			ryc;	/*[106] Yc分解能[m]								*/
	double			rzc;	/*[114] Zc分解能[m]								*/
	double			rb;		/*[122] B 分解能[-]								*/
	unsigned long	vx;		/*[130] X 速度 speed						*/
	unsigned long	vy;		/*[134] Y 速度									*/
	unsigned long	vz;		/*[138] Z 速度									*/
	unsigned long	vv;		/*[142] V 速度									*/
	unsigned long	vxc;	/*[146] Xc速度									*/
	unsigned long	vyc;	/*[150] Yc速度									*/
	unsigned long	vzc;	/*[154] Zc速度									*/
	unsigned long	vb;		/*[158] B 速度									*/
	unsigned short		gi;		/*[162] ゲイン(I) gain					*/
	unsigned short		gf;		/*[164] ゲイン(F)	 "						*/
	unsigned short		gm;		/*[166] ゲイン(M)	 "						*/
	unsigned short		ga;		/*[168] ゲイン(A)	 "						*/

	unsigned short		dt;		/*[170] データ取得開始 日付	date (begin of scanning) 	*/
	unsigned short		tm;		/*[172] データ取得開始 時刻	time (begin of scanning)	*/
	unsigned long	ta;		/*[174] データ取得時間[100us]	duration of scanning */
	unsigned long	ts;		/*[178] システム時刻[100us]	sytem time?					*/

	char			s1[8];				/*[182] --------								*/

	unsigned short		nf;		/*[190] フレーム数 Frame counter */
	unsigned short		td;		/*[192] データ種(bit0-15)		(*1)			*/
	unsigned short		nx;		/*[194] X 方向ピクセル数 number of pixel w	*/
	unsigned short		ny;		/*[196] Y 方向ピクセル数 number of pixel h	*/
	unsigned short		dp;		/*[198] ピクセル間隔 space between pixel??	*/
	unsigned short		ns;		/*[200] 走査範囲 Space between scan lines??	*/
	short				an;					/*[202] 走査角度[/100degree](-18000 - 17999)	*/
	unsigned short		ia;		/*[204] 積算回数 Time for 1 schanposhort			*/
	unsigned short		fc;		/*[206] FB制御量 Feedback				(*1)			*/

	char			s2[12];				/*[208] --------								*/

	unsigned short		dm;		/*[220] 測定間隔 measurement shorterval	*/
	unsigned short		ty;		/*[222] データ種(bit0-15)		(*1)	kind of data (see comment below) */
	unsigned short		tx;		/*[224] 横軸(0-16)				(*1) kind of axis value (see comment below) */
	long			sd;						/*[226] 横軸始点 origin X	*/
	unsigned long	nd;				/*[230] 横軸点数 number x? */
	long			dd;						/*[234] 横軸間隔	x-axis width */
	unsigned short		ib;		/*[238] 積算回数 total number?*/

	char			s3[12];	/*[240] --------								*/

	char far*		pn;		/*[252] 画像ファイル名へのポインタ(データ共有用) Next Header?*/
	} HitachiSpm;

/* (*1)データ種	(00) I  (01) F  (02) M  (03) A  (04) X  (05) Y  (06) Z  (07) V
				(08) Xc (09) Yc (10) Zc (11) B  (12) Id (13) Fd (14) Md (15) Ad
				(16) t  (17) NO												*/

