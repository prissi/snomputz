/******** �v���g�^�C�v (plib.lib)	11/17/2000  by S.Heike *******************/

typedef struct		/* Header for Hitachi-AFM */
	{
	unsigned short	id;		/* [000] �f�[�^���ʎq(0100h)	data offset	*/
	short				ai;		/*[002] I ���ݒl current preset	*/
	short				af;		/*[004] F ���ݒl force					*/
	short				am;		/*[006] M ���ݒl								*/
	short				aa;		/*[008] A ���ݒl								*/
	short				dx;		/*[010] X ���ݒl offset?				*/
	short				dy;		/*[012] Y ���ݒl								*/
	short				dz;		/*[014] Z ���ݒl								*/
	short				dv;		/*[016] V ���ݒl								*/
	short				dxc;	/*[018] Xc���ݒl								*/
	short				dyc;	/*[020] Yc���ݒl								*/
	short				dzc;	/*[022] Zc���ݒl								*/
	short				db;		/*[024] B ���ݒl								*/
	short				si;		/*[026] I �ݒ�l init						*/
	short				sf;		/*[028] F �ݒ�l								*/
	short				sm;		/*[030] M �ݒ�l								*/
	short				sa;		/*[032] A �ݒ�l								*/
	double			ri;		/*[034] I ����\[A]	resolution		*/
	double			rf;		/*[042] F ����\[N]								*/
	double			rm;		/*[050] M ����\[Hz]							*/
	double			ra;		/*[058] A ����\[-]								*/
	double			rx;		/*[066] X ����\[m]	scaling factor */
	double			ry;		/*[074] Y ����\[m]								*/
	double			rz;		/*[082] Z ����\[m]								*/
	double			rv;		/*[090] V ����\[V]								*/
	double			rxc;	/*[098] Xc����\[m]	unit in m			*/
	double			ryc;	/*[106] Yc����\[m]								*/
	double			rzc;	/*[114] Zc����\[m]								*/
	double			rb;		/*[122] B ����\[-]								*/
	unsigned long	vx;		/*[130] X ���x speed						*/
	unsigned long	vy;		/*[134] Y ���x									*/
	unsigned long	vz;		/*[138] Z ���x									*/
	unsigned long	vv;		/*[142] V ���x									*/
	unsigned long	vxc;	/*[146] Xc���x									*/
	unsigned long	vyc;	/*[150] Yc���x									*/
	unsigned long	vzc;	/*[154] Zc���x									*/
	unsigned long	vb;		/*[158] B ���x									*/
	unsigned short		gi;		/*[162] �Q�C��(I) gain					*/
	unsigned short		gf;		/*[164] �Q�C��(F)	 "						*/
	unsigned short		gm;		/*[166] �Q�C��(M)	 "						*/
	unsigned short		ga;		/*[168] �Q�C��(A)	 "						*/

	unsigned short		dt;		/*[170] �f�[�^�擾�J�n ���t	date (begin of scanning) 	*/
	unsigned short		tm;		/*[172] �f�[�^�擾�J�n ����	time (begin of scanning)	*/
	unsigned long	ta;		/*[174] �f�[�^�擾����[100us]	duration of scanning */
	unsigned long	ts;		/*[178] �V�X�e������[100us]	sytem time?					*/

	char			s1[8];				/*[182] --------								*/

	unsigned short		nf;		/*[190] �t���[���� Frame counter */
	unsigned short		td;		/*[192] �f�[�^��(bit0-15)		(*1)			*/
	unsigned short		nx;		/*[194] X �����s�N�Z���� number of pixel w	*/
	unsigned short		ny;		/*[196] Y �����s�N�Z���� number of pixel h	*/
	unsigned short		dp;		/*[198] �s�N�Z���Ԋu space between pixel??	*/
	unsigned short		ns;		/*[200] �����͈� Space between scan lines??	*/
	short				an;					/*[202] �����p�x[/100degree](-18000 - 17999)	*/
	unsigned short		ia;		/*[204] �ώZ�� Time for 1 schanposhort			*/
	unsigned short		fc;		/*[206] FB����� Feedback				(*1)			*/

	char			s2[12];				/*[208] --------								*/

	unsigned short		dm;		/*[220] ����Ԋu measurement shorterval	*/
	unsigned short		ty;		/*[222] �f�[�^��(bit0-15)		(*1)	kind of data (see comment below) */
	unsigned short		tx;		/*[224] ����(0-16)				(*1) kind of axis value (see comment below) */
	long			sd;						/*[226] �����n�_ origin X	*/
	unsigned long	nd;				/*[230] �����_�� number x? */
	long			dd;						/*[234] �����Ԋu	x-axis width */
	unsigned short		ib;		/*[238] �ώZ�� total number?*/

	char			s3[12];	/*[240] --------								*/

	char far*		pn;		/*[252] �摜�t�@�C�����ւ̃|�C���^(�f�[�^���L�p) Next Header?*/
	} HitachiSpm;

/* (*1)�f�[�^��	(00) I  (01) F  (02) M  (03) A  (04) X  (05) Y  (06) Z  (07) V
				(08) Xc (09) Yc (10) Zc (11) B  (12) Id (13) Fd (14) Md (15) Ad
				(16) t  (17) NO												*/

