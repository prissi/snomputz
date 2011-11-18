#ifdef WIN32
#define far
#endif

// Polynom n-ter Ordnung für MittelFitBild ...
void	fpoly( double x, LPDOUBLE p, int n );

// Polynom n-ter Ordnung für MittelFitBild ...
double	fPolyWert( double x, LPDOUBLE p, int n );

BOOL lfit(int HUGE *x, LPUWORD y, int ndat, LPDOUBLE a, int ma,
					double HUGE * HUGE *covar, LPDOUBLE chisq, void (HUGE *funcs)(double, LPDOUBLE, int));
