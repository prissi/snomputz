


void
render_to_avi(char *aviname)
{
	HDC hdcscreen=GetDC(0), hdc=CreateCompatibleDC(hdcscreen); ReleaseDC(0,hdcscreen);
	BITMAPINFO bi; ZeroMemory(&bi,sizeof(bi)); BITMAPINFOHEADER &bih = bi.bmiHeader;
	bih.biSize=sizeof(bih);
	bih.biWidth=200;
	bih.biHeight=200;
	bih.biPlanes=1;
	bih.biBitCount=24;
	bih.biCompression=BI_RGB;
	bih.biSizeImage = ((bih.biWidth*bih.biBitCount/8+3)&0xFFFFFFFC)*bih.biHeight;
	bih.biXPelsPerMeter=10000;
	bih.biYPelsPerMeter=10000;
	bih.biClrUsed=0;
	bih.biClrImportant=0;
	void *bits; HBITMAP hbm=CreateDIBSection(hdc,(BITMAPINFO*)&bih,DIB_RGB_COLORS,&bits,NULL,NULL);
	//
	HGDIOBJ holdb=SelectObject(hdc,hbm);
	HPEN hp = CreatePen(PS_SOLID,16,RGB(255,255,128));
	HGDIOBJ holdp=SelectObject(hdc,hp);
	//
	HAVI avi = CreateAvi("test.avi",100,NULL);
	for (int frame=0; frame<50; frame++)
	{ // static background
		DWORD seed=GetTickCount(); DWORD *dbits=(DWORD*)bits;
		for (unsigned int i=0; i<bih.biSizeImage/sizeof(DWORD); i++) {dbits[i]=seed; seed+=79;}
		// a line moving
		MoveToEx(hdc,0,0,NULL); LineTo(hdc,frame*3,100);
		AddAviFrame(avi,hbm);
	}
	CloseAvi(avi);
	//
	SelectObject(hdc,holdb); SelectObject(hdc,holdp);
	DeleteDC(hdc); DeleteObject(hbm); DeleteObject(hp);
}