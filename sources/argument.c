/*
  	#[ include : argument.c
*/

#include "form3.h"

static UWORD *EAscrat = 0;

/*
  	#] include : 
  	#[ execarg :

	Executes the subset of statements in an argument environment.
	The calling routine should be of the type
	if ( C->lhs[level][0] == TYPEARG ) {
		if ( execarg(term,level) ) goto GenCall;
		level = C->lhs[level][2];
		goto SkipCount;
	}
	Note that there will be cases that extra space is needed.
	In addition the compare with C->numlhs isn't very fine, because we
	need to insert a different value (C->lhs[level][2]).
*/

WORD mulpat[] = {TYPEMULT, SUBEXPSIZE+3, 0,
						SUBEXPRESSION, SUBEXPSIZE, 0, 1, 0, 0, 0 };
WORD locwildvalue[SUBEXPSIZE] = { SUBEXPRESSION, SUBEXPSIZE, 0, 0, 0 };

static UWORD *GCDbuffer = 0, *LCMbuffer = 0, *LCMb = 0, *LCMc = 0;

WORD
execarg ARG2(WORD *,term,WORD,level)
{
	WORD *t, *r, *m, *v;
	WORD *start, *stop, *rstop, *r1, *r2 = 0, *r3 = 0, *r4, *r5, *r6, *r7, *r8, *r9;
	WORD *mm, *mstop, *mnext, *rnext, *rr, *factor, type, ngcd, nq;
	CBUF *C = cbuf+AM.rbufnum, *CC = cbuf+AR.ebufnum;
	WORD i, j, k, oldnumlhs = C->numlhs, count, action = 0, olddefer = AR.DeferFlag;
	WORD oldnumrhs = CC->numrhs, size, pow, ncom, jj;
	LONG oldcpointer = CC->Pointer - CC->Buffer, oldppointer = AR.pWorkPointer, lp;
	WORD *oldwork = AR.WorkPointer, *oldwork2, scale, renorm;
	WORD kLCM = 0, kGCD = 0, kkLCM = 0, jLCM = 0, jGCD;
	AR.WorkPointer += *term;
	start = C->lhs[level];
	C->numlhs = start[2];
	stop = start + start[1];
	type = *start;
	scale = start[4];
	renorm = start[5];
	start += TYPEARGHEADSIZE;
	if ( renorm && start[1] != 0 ) {/* We have to evaluate $ symbols inside () */
		t = start+1; factor = oldwork2 = v = AR.WorkPointer;
		i = *t; t++;
		*v++ = i+3; i--; NCOPY(v,t,i);
		*v++ = 1; *v++ = 1; *v++ = 3;
		AR.WorkPointer = v;
		start = t; AC.Eside = LHSIDEX;
		NewSort();
		if ( Generator(factor,C->numlhs) ) {
			LowerSortLevel();
			AR.WorkPointer = oldwork;
			return(-1);
		}
		AR.WorkPointer = v;
		if ( EndSort(factor,0) < 0 ) {}
		if ( *factor && *(factor+*factor) != 0 ) {
			MesPrint("&$ in () does not evaluate into a single term");
			return(-1);
		}
		AC.Eside = RHSIDE;
		if ( *factor > 0 ) {
			v = factor+*factor;
			v -= ABS(v[-1]);
			*factor = v-factor;
		}
		AR.WorkPointer = v;
	}
	else {
		if ( *start < 0 ) {
			factor = start + 1;
			start += -*start;
		}
		else factor = 0;
	}
	t = term;
	r = t + *t;
	rstop = r - ABS(r[-1]);
	t++;
	while ( t < rstop ) {
		if ( *t >= FUNCTION && functions[*t-FUNCTION].spec == 0 ) {
/*
			We have a function. First count the number of arguments.
			Tensors are excluded.
*/
			count = 0;
			v = t;
			m = t + FUNHEAD;
			r = t + t[1];
			while ( m < r ) {
				count++;
				NEXTARG(m)
			}
			if ( count <= 0 ) { t += t[1]; continue; }
/*
			Now we take the arguments one by one and test for a match
*/
			for ( i = 1; i <= count; i++ ) {
				m = start;
				while ( m < stop ) {
					r = m + m[1];
					j = *r++;
					if ( j > 1 ) {
						while ( --j > 0 ) {
							if ( *r == i ) goto RightNum;
							r++;
						}
						m = r;
						continue;
					}
RightNum:
					if ( m[1] == 2 ) {
						m += 2;
						m += *m;
						goto HaveTodo;
					}
					else {
						r = m + m[1];
						m += 2;
						while ( m < r ) {
							if ( *m == CSET ) {
								r1 = SetElements + Sets[m[1]].first;
								r2 = SetElements + Sets[m[1]].last;
								while ( r1 <= r2 ) {
									if ( *r1++ == *t ) goto HaveTodo;
								}
							}
							else if ( m[1] == *t ) goto HaveTodo;
							m += 2;
						}
					}
					m += *m;
				}
				continue;
HaveTodo:
/*
				If we come here we have to do the argument i (first is 1).
*/
				v[2] |= DIRTYFLAG;
				r = t + FUNHEAD;
				j = i;
				while ( --j > 0 ) { NEXTARG(r) }
				if ( ( type == TYPESPLITARG ) || ( type == TYPESPLITFIRSTARG )
				 || ( type == TYPESPLITLASTARG ) ) {
					if ( *t > FUNCTION && *r > 0 ) {
						WantAddPointers(2);
						AR.pWorkSpace[AR.pWorkPointer++] = t;
						AR.pWorkSpace[AR.pWorkPointer++] = r;
					}
					continue;
				}
				else if ( type == TYPESPLITARG2 ) {
					if ( *t > FUNCTION && *r > 0 ) {
						WantAddPointers(2);
						AR.pWorkSpace[AR.pWorkPointer++] = t;
						AR.pWorkSpace[AR.pWorkPointer++] = r;
					}
					continue;
				}
				else if ( type == TYPEFACTARG || type == TYPEFACTARG2 ) {
					if ( *t > FUNCTION || *t == DENOMINATOR ) {
						if ( *r > 0 ) {
						mm = r + ARGHEAD; mstop = r + *r;
						if ( mm + *mm < mstop ) {
							WantAddPointers(2);
							AR.pWorkSpace[AR.pWorkPointer++] = t;
							AR.pWorkSpace[AR.pWorkPointer++] = r;
							continue;
						}
						if ( *mm == 1+ABS(mstop[-1]) ) continue;
						if ( mstop[-3] != 1 || mstop[-2] != 1
							|| mstop[-1] != 3 ) {
							WantAddPointers(2);
							AR.pWorkSpace[AR.pWorkPointer++] = t;
							AR.pWorkSpace[AR.pWorkPointer++] = r;
							continue;
						}
						GETSTOP(mm,mstop); mm++;
						if ( mm + mm[1] < mstop ) {
							WantAddPointers(2);
							AR.pWorkSpace[AR.pWorkPointer++] = t;
							AR.pWorkSpace[AR.pWorkPointer++] = r;
							continue;
						}
						if ( *mm == SYMBOL && ( mm[1] > 4 ||
							( mm[3] != 1 && mm[3] != -1 ) ) ) {
							WantAddPointers(2);
							AR.pWorkSpace[AR.pWorkPointer++] = t;
							AR.pWorkSpace[AR.pWorkPointer++] = r;
							continue;
						}
						else if ( *mm == DOTPRODUCT && ( mm[1] > 5 ||
							( mm[4] != 1 && mm[4] != -1 ) ) ) {
							WantAddPointers(2);
							AR.pWorkSpace[AR.pWorkPointer++] = t;
							AR.pWorkSpace[AR.pWorkPointer++] = r;
							continue;
						}
						else if ( ( *mm == DELTA || *mm == VECTOR )
							 && mm[1] > 4 ) {
							WantAddPointers(2);
							AR.pWorkSpace[AR.pWorkPointer++] = t;
							AR.pWorkSpace[AR.pWorkPointer++] = r;
							continue;
						}
						}
						else if ( factor && *factor == 4 && factor[2] == 1 ) {
							WantAddPointers(2);
							AR.pWorkSpace[AR.pWorkPointer++] = t;
							AR.pWorkSpace[AR.pWorkPointer++] = r;
							continue;
						}
						else if ( factor && *factor == 0
						&& *r == -SNUMBER && r[1] != 1 ) {
							WantAddPointers(2);
							AR.pWorkSpace[AR.pWorkPointer++] = t;
							AR.pWorkSpace[AR.pWorkPointer++] = r;
							continue;
						}
					}
					continue;
				}
				else if ( type == TYPENORM || type == TYPENORM2 || type == TYPENORM3 || type == TYPENORM4 ) {
					if ( *r < 0 ) {
						if ( *r != -SNUMBER || r[1] == 1 || r[1] == 0 ) continue;
/*
						Now we must multiply the general coefficient by r[1]
*/
						if ( scale && ( factor == 0 || *factor ) ) {
							v[2] |= DIRTYFLAG;
							if ( r[1] < 0 ) {
								if ( type == TYPENORM3 ) k = 1;
								else k = -1;
								r[1] = -r[1];
							}
							else k = 1;
							r1 = term + *term;
							size = r1[-1];
							size = REDLENG(size);
							if ( scale > 0 ) {
								for ( jj = 0; jj < scale; jj++ ) {
									if ( Mully((UWORD *)rstop,&size,(UWORD *)(r+1),k) )
										goto execargerr;
								}
							}
							else {
								for ( jj = 0; jj > scale; jj-- ) {
									if ( Divvy((UWORD *)rstop,&size,(UWORD *)(r+1),k) )
										goto execargerr;
								}
							}
							size = INCLENG(size);
							k = size < 0 ? -size: size;
							rstop[k-1] = size;
							*term = (WORD)(rstop - term) + k;
						}
						r[1] = 1;
						continue;
					}
/*
					Now we have to find a reference term.
					If factor is defined and *factor != 0 we have to
					look for the first term that matches the pattern exactly
					Otherwise the first term plays this role
					If its coefficient is not one,
					we must set up a division of the whole argument by
					this coefficient, and a multiplication of the term
					when the type is not equal to TYPENORM2.
					We first multiply the coefficient of the term.
					Then we set up the division.

					First find the magic term
*/
					if ( type == TYPENORM4 ) {
/*
						For normalizing everything to integers we have to
						determine for all elements of this argument the LCM of
						the denominators and the GCD of the numerators.
*/
						if ( GCDbuffer == 0 ) {
							GCDbuffer = (UWORD *)Malloc1(4*(AM.MaxTal+2)*sizeof(UWORD),"execarg");
							LCMbuffer = GCDbuffer + AM.MaxTal+2;
							LCMb = LCMbuffer + AM.MaxTal+2;
							LCMc = LCMb + AM.MaxTal+2;
						}
						r4 = r + *r;
						r1 = r + ARGHEAD;
/*
						First take the first term to load up the LCM and the GCD
*/
						r2 = r1 + *r1;
						j = r2[-1];
						r3 = r2 - ABS(j);
						k = REDLENG(j);
						if ( k < 0 ) k = -k;
						while ( ( k > 1 ) && ( r3[k-1] == 0 ) ) k--;
						for ( kGCD = 0; kGCD < k; kGCD++ ) GCDbuffer[kGCD] = r3[kGCD];
						k = REDLENG(j);
						if ( k < 0 ) k = -k;
						r3 += k;
						while ( ( k > 1 ) && ( r3[k-1] == 0 ) ) k--;
						for ( kLCM = 0; kLCM < k; kLCM++ ) LCMbuffer[kLCM] = r3[kLCM];
						r1 = r2;
/*
						Now go through the rest of the terms in this argument.
*/
						while ( r1 < r4 ) {
							r2 = r1 + *r1;
							j = r2[-1];
							r3 = r2 - ABS(j);
							k = REDLENG(j);
							if ( k < 0 ) k = -k;
							while ( ( k > 1 ) && ( r3[k-1] == 0 ) ) k--;
							if ( ( ( GCDbuffer[0] == 1 ) && ( kGCD == 1 ) ) ) {
/*
								GCD is already 1
*/
							}
							else if ( ( ( k != 1 ) || ( r3[0] != 1 ) ) ) {
								if ( GcdLong(GCDbuffer,kGCD,(UWORD *)r3,k,GCDbuffer,&kGCD) ) {
									goto execargerr;
								}
							}
							else {
								kGCD = 1; GCDbuffer[0] = 1;
							}
							k = REDLENG(j);
							if ( k < 0 ) k = -k;
							r3 += k;
							while ( ( k > 1 ) && ( r3[k-1] == 0 ) ) k--;
							if ( ( ( LCMbuffer[0] == 1 ) && ( kLCM == 1 ) ) ) {
								for ( kLCM = 0; kLCM < k; kLCM++ )
									LCMbuffer[kLCM] = r3[kLCM];
							}
							else if ( ( k != 1 ) || ( r3[0] != 1 ) ) {
								if ( GcdLong(LCMbuffer,kLCM,(UWORD *)r3,k,LCMb,&kkLCM) ) {
									goto execargerr;
								}
								DivLong((UWORD *)r3,k,LCMb,kkLCM,LCMb,&kkLCM,LCMc,&jLCM);
								MulLong(LCMbuffer,kLCM,LCMb,kkLCM,LCMc,&jLCM);
								for ( kLCM = 0; kLCM < jLCM; kLCM++ )
									LCMbuffer[kLCM] = LCMc[kLCM];
							}
							else {} /* LCM doesn't change */
							r1 = r2;
						}
/*
						Now put the factor together: GCD/LCM
*/
						r3 = (WORD *)(GCDbuffer);
						if ( kGCD == kLCM ) {
							for ( jGCD = 0; jGCD < kGCD; jGCD++ )
								r3[jGCD+kGCD] = LCMbuffer[jGCD];
							k = kGCD;
						}
						else if ( kGCD > kLCM ) {
							for ( jGCD = 0; jGCD < kLCM; jGCD++ )
								r3[jGCD+kGCD] = LCMbuffer[jGCD];
							for ( jGCD = kLCM; jGCD < kGCD; jGCD++ )
								r3[jGCD+kGCD] = 0;
							k = kGCD;
						}
						else {
							for ( jGCD = kGCD; jGCD < kLCM; jGCD++ )
								r3[jGCD] = 0;
							for ( jGCD = 0; jGCD < kLCM; jGCD++ )
								r3[jGCD+kLCM] = LCMbuffer[jGCD];
							k = kLCM;
						}
						j = 2*k+1;
/*
						Now we have to correct the overal factor
*/
						size = term[*term-1];
						size = REDLENG(size);
						if ( MulRat((UWORD *)rstop,size,(UWORD *)r3,k,
								(UWORD *)rstop,&size) ) goto execargerr;
						size = INCLENG(size);
						k = size < 0 ? -size: size;
						rstop[k-1] = size;
						*term = (WORD)(rstop - term) + k;
					}
					else {
						if ( factor && *factor >= 1 ) {
							r4 = r + *r;
							r1 = r + ARGHEAD;
							while ( r1 < r4 ) {
								r2 = r1 + *r1;
								r3 = r2 - ABS(r2[-1]);
								j = r3 - r1;
								r5 = factor;
								if ( j != *r5 ) { r1 = r2; continue; }
								r5++; r6 = r1+1;
								while ( --j > 0 ) {
									if ( *r5 != *r6 ) break;
									r5++; r6++;
								}
								if ( j > 0 ) { r1 = r2; continue; }
								break;
							}
							if ( r1 >= r4 ) continue;
						}
						else {
							r1 = r + ARGHEAD;
							r2 = r1 + *r1;
							r3 = r2 - ABS(r2[-1]);
						}
						if ( *r3 == 1 && r3[1] == 1 ) {
							if ( r2[-1] == 3 ) continue;
							if ( r2[-1] == -3 && type == TYPENORM3 ) continue;
						}
						v[2] |= DIRTYFLAG;
						j = r2[-1];
						k = REDLENG(j);
						if ( j < 0 ) j = -j;
						if ( type == TYPENORM && scale && ( factor == 0 || *factor ) ) {
/*
							Now we correct the overal factor
*/
							size = term[*term-1];
							size = REDLENG(size);
							if ( scale > 0 ) {
								for ( jj = 0; jj < scale; jj++ ) {
									if ( MulRat((UWORD *)rstop,size,(UWORD *)r3,k,
										(UWORD *)rstop,&size) ) goto execargerr;
								}
							}
							else {
								for ( jj = 0; jj > scale; jj-- ) {
									if ( DivRat((UWORD *)rstop,size,(UWORD *)r3,k,
										(UWORD *)rstop,&size) ) goto execargerr;
								}
							}
							size = INCLENG(size);
							k = size < 0 ? -size: size;
							rstop[k-1] = size;
							*term = (WORD)(rstop - term) + k;
						}
					}
/*
                  	We generate a statement for addapting all terms in the
					argument sucessively
*/
					r4 = AddRHS(AR.ebufnum,1);
					while ( (r4+j+12) > CC->Top ) r4 = DoubleCbuffer(AR.ebufnum,r4);
					*r4++ = j+1;
					i = (j-1)>>1;
					for ( k = 0; k < i; k++ ) *r4++ = r3[i+k];
					for ( k = 0; k < i; k++ ) *r4++ = r3[k];
					if ( ( type == TYPENORM3 ) || ( type == TYPENORM4 ) ) *r4++ = j;
					else *r4++ = r3[j-1];
					*r4++ = 0;
					CC->rhs[CC->numrhs+1] = r4;
					CC->Pointer = r4;
					mulpat[5] = CC->numrhs;
					mulpat[7] = AR.ebufnum;
				}
				r3 = r;
				AR.DeferFlag = 0;
				if ( *r > 0 ) {
					NewSort();
					action = 1;
					r2 = r + *r;
					r += ARGHEAD;
					while ( r < r2 ) {	/* Sum over the terms */
						m = AR.WorkPointer;
						j = *r;
						while ( --j >= 0 ) *m++ = *r++;
						r1 = AR.WorkPointer;
						AR.WorkPointer = m;
/*
						What to do with dummy indices?
*/
						if ( type == TYPENORM || type == TYPENORM2 || type == TYPENORM3 || type == TYPENORM4 ) {
							if ( MultDo(r1,mulpat) ) goto execargerr;
							AR.WorkPointer = r1 + *r1;
						}
						if ( Generator(r1,level) ) goto execargerr;
						AR.WorkPointer = r1;
					}
				}
				else {
					r2 = r + (( *r <= -FUNCTION ) ? 1:2);
					r1 = AR.WorkPointer;
					ToGeneral(r,r1,0);
					m = r1 + ARGHEAD;
					AR.WorkPointer = r1 + *r1;
					NewSort();
					action = 1;
/*
					What to do with dummy indices?
*/
					if ( type == TYPENORM || type == TYPENORM2 || type == TYPENORM3 || type == TYPENORM4 ) {
						if ( MultDo(m,mulpat) ) goto execargerr;
						AR.WorkPointer = m + *m;
					}
					if ( Generator(m,level) ) goto execargerr;
					AR.WorkPointer = r1;
				}
				if ( EndSort(AR.WorkPointer+ARGHEAD,1) < 0 ) goto execargerr;
				AR.DeferFlag = olddefer;
/*
				Now shift the sorted entity over the old argument.
*/
				m = AR.WorkPointer+ARGHEAD;
				while ( *m ) m += *m;
				k = WORDDIF(m,AR.WorkPointer);
				*AR.WorkPointer = k;
				AR.WorkPointer[1] = 0;
				if ( ToFast(AR.WorkPointer,AR.WorkPointer) ) {
					if ( *AR.WorkPointer <= -FUNCTION ) k = 1;
					else k = 2;
				}
				
				if ( *r3 > 0 ) j = k - *r3;
				else if ( *r3 <= -FUNCTION ) j = k - 1;
				else j = k - 2;

				t[1] += j;
				v[2] |= DIRTYFLAG;
				if ( j > 0 ) {
					r = m + j;
					while ( m > AR.WorkPointer ) *--r = *--m;
					AR.WorkPointer = r;
					m = term + *term;
					r = m + j;
					while ( m > r2 ) *--r = *--m;
				}
				else if ( j < 0 ) {
					r = r2 + j;
					r1 = term + *term;
					while ( r2 < r1 ) *r++ = *r2++;
				}
				r = r3;
				m = AR.WorkPointer;
				NCOPY(r,m,k);
				*term += j;
				rstop += j;
				CC->numrhs = oldnumrhs;
				CC->Pointer = CC->Buffer + oldcpointer;
			}
		}
		t += t[1];
	}
	if ( ( type == TYPESPLITARG || type == TYPESPLITARG2
	 || type == TYPESPLITFIRSTARG || type == TYPESPLITLASTARG ) && 
	AR.pWorkPointer > oldppointer ) {
		t = term+1;
		r1 = AR.WorkPointer + 1;
		lp = oldppointer;
		while ( t < rstop ) {
			if ( lp < AR.pWorkPointer && t == AR.pWorkSpace[lp] ) {
				v = t;
				m = t + FUNHEAD;
				r = t + t[1];
				r2 = r1; while ( t < m ) *r1++ = *t++;
				while ( m < r ) {
					t = m;
					NEXTARG(m)
					if ( lp >= AR.pWorkPointer || t != AR.pWorkSpace[lp+1] ) {
						if ( *t > 0 ) t[1] = 0;
						while ( t < m ) *r1++ = *t++;
						continue;
					}
/*
					Now we have a nontrivial argument that should be done.
*/
					lp += 2;
					action = 1;
					v[2] |= DIRTYFLAG;
					r3 = t + *t;
					t += ARGHEAD;
					if ( type == TYPESPLITFIRSTARG ) {
						r4 = r1; r5 = t; r7 = oldwork;
						*r1++ = *t + ARGHEAD;
						for ( i = 1; i < ARGHEAD; i++ ) *r1++ = 0;
						j = 0;
						while ( t < r3 ) {
							i = *t;
							if ( j == 0 ) {
								NCOPY(r7,t,i)
								j++;
							}
							else {
								NCOPY(r1,t,i)
							}
						}
						*r4 = r1 - r4;
						if ( j ) {
							if ( ToFast(r4,r4) ) {
								r1 = r4;
								if ( *r1 > -FUNCTION ) r1++;
								r1++;
							}
							r7 = oldwork;
							while ( --j >= 0 ) {
								r4 = r1; i = *r7;
								*r1++ = i+ARGHEAD; *r1++ = 0;
								FILLARG(r1);
								NCOPY(r1,r7,i)
								if ( ToFast(r4,r4) ) {
									r1 = r4;
									if ( *r1 > -FUNCTION ) r1++;
									r1++;
								}
							}
						}
						t = r3;
					}
					else if ( type == TYPESPLITLASTARG ) {
						r4 = r1; r5 = t; r7 = oldwork;
						*r1++ = *t + ARGHEAD;
						for ( i = 1; i < ARGHEAD; i++ ) *r1++ = 0;
						j = 0;
						while ( t < r3 ) {
							i = *t;
							if ( t+i >= r3 ) {
								NCOPY(r7,t,i)
								j++;
							}
							else {
								NCOPY(r1,t,i)
							}
						}
						*r4 = r1 - r4;
						if ( j ) {
							if ( ToFast(r4,r4) ) {
								r1 = r4;
								if ( *r1 > -FUNCTION ) r1++;
								r1++;
							}
							r7 = oldwork;
							while ( --j >= 0 ) {
								r4 = r1; i = *r7;
								*r1++ = i+ARGHEAD; *r1++ = 0;
								FILLARG(r1);
								NCOPY(r1,r7,i)
								if ( ToFast(r4,r4) ) {
									r1 = r4;
									if ( *r1 > -FUNCTION ) r1++;
									r1++;
								}
							}
						}
						t = r3;
					}
					else if ( factor == 0 || ( type == TYPESPLITARG2 && *factor == 0 ) ) {
						while ( t < r3 ) {
							r4 = r1;
							*r1++ = *t + ARGHEAD;					
							for ( i = 1; i < ARGHEAD; i++ ) *r1++ = 0;
							i = *t;
							while ( --i >= 0 ) *r1++ = *t++;
							if ( ToFast(r4,r4) ) {
								r1 = r4;
								if ( *r1 > -FUNCTION ) r1++;
								r1++;
							}
						}
					}
					else if ( type == TYPESPLITARG2 ) {
/*
						Here we better put the pattern matcher at work?
						Remember: there are no wildcards.
*/
						WORD *oRepFunList = AN.RepFunList;
						WORD *oWildMask = AN.WildMask, *oWildValue = AN.WildValue;
						AN.WildValue = locwildvalue; AN.WildMask = locwildvalue+2;
						AN.NumWild = 0;
						r4 = r1; r5 = t; r7 = oldwork;
						*r1++ = *t + ARGHEAD;
						for ( i = 1; i < ARGHEAD; i++ ) *r1++ = 0;
						j = 0;
						while ( t < r3 ) {
							AN.UseFindOnly = 0; oldwork2 = AR.WorkPointer;
							AN.RepFunList = r1;
							AR.WorkPointer = r1+AN.RepFunNum+2;
							i = *t;
							if ( FindRest(t,factor) &&
							 ( AN.UsedOtherFind || FindOnce(t,factor) ) ) {
								NCOPY(r7,t,i)
								j++;
							}
							else if ( factor[0] == FUNHEAD+1 && factor[1] >= FUNCTION ) {
								WORD *rr1 = t+1, *rr2 = t+i;
								rr2 -= ABS(rr2[-1]);
								while ( rr1 < rr2 ) {
									if ( *rr1 == factor[1] ) break;
									rr1 += rr1[1];
								}
								if ( rr1 < rr2 ) {
									NCOPY(r7,t,i)
									j++;
								}
								else {
									NCOPY(r1,t,i)
								}
							}
							else {
								NCOPY(r1,t,i)
							}
							AR.WorkPointer = oldwork2;
						}
						AN.RepFunList = oRepFunList;
						*r4 = r1 - r4;
						if ( j ) {
							if ( ToFast(r4,r4) ) {
								r1 = r4;
								if ( *r1 > -FUNCTION ) r1++;
								r1++;
							}
							r7 = oldwork;
							while ( --j >= 0 ) {
								r4 = r1; i = *r7;
								*r1++ = i+ARGHEAD; *r1++ = 0;
								FILLARG(r1);
								NCOPY(r1,r7,i)
								if ( ToFast(r4,r4) ) {
									r1 = r4;
									if ( *r1 > -FUNCTION ) r1++;
									r1++;
								}
							}
						}
						t = r3;
						AN.WildMask = oWildMask; AN.WildValue = oWildValue;
					}
					else {
/*
						This code deals with splitting off a single term
*/
						r4 = r1; r5 = t;
						*r1++ = *t + ARGHEAD;
						for ( i = 1; i < ARGHEAD; i++ ) *r1++ = 0;
						j = 0;
						while ( t < r3 ) {
							r6 = t + *t; r6 -= ABS(r6[-1]);
							if ( (r6 - t) == *factor ) {
								k = *factor - 1;
								for ( ; k > 0; k-- ) {
									if ( t[k] != factor[k] ) break;
								}
								if ( k <= 0 ) {
									j = r3 - t; t += *t; continue;
								}
							}
							else if ( (r6 - t) == 1 && *factor == 0 ) {
								j = r3 - t; t += *t; continue;
							}
							i = *t;
							NCOPY(r1,t,i)
						}
						*r4 = r1 - r4;
						if ( j ) {
							if ( ToFast(r4,r4) ) {
								r1 = r4;
								if ( *r1 > -FUNCTION ) r1++;
								r1++;
							}
							t = r3 - j;
							r4 = r1;
							*r1++ = *t + ARGHEAD;
							for ( i = 1; i < ARGHEAD; i++ ) *r1++ = 0;
							i = *t;
							while ( --i >= 0 ) *r1++ = *t++;
							if ( ToFast(r4,r4) ) {
								r1 = r4;
								if ( *r1 > -FUNCTION ) r1++;
								r1++;
							}
						}
						t = r3;
					}
				}
				r2[1] = r1 - r2;
			}
			else {
				r = t + t[1];
				while ( t < r ) *r1++ = *t++;
			}
		}
		r = term + *term;
		while ( t < r ) *r1++ = *t++;
		m = AR.WorkPointer;
		i = m[0] = r1 - m;
		t = term;
		while ( --i >= 0 ) *t++ = *m++;
		if ( AR.WorkPointer < m ) AR.WorkPointer = m;
	}
	if ( ( type == TYPEFACTARG || type == TYPEFACTARG2 ) && 
	AR.pWorkPointer > oldppointer ) {
		t = term+1;
		r1 = AR.WorkPointer + 1;
		lp = oldppointer;
		while ( t < rstop ) {
			if ( lp < AR.pWorkPointer && AR.pWorkSpace[lp] == t ) {
				v = t;
				m = t + FUNHEAD;
				r = t + t[1];
				r2 = r1; while ( t < m ) *r1++ = *t++;
				while ( m < r ) {
					rr = t = m;
					NEXTARG(m)
					if ( lp >= AR.pWorkPointer || AR.pWorkSpace[lp+1] != t ) {
						if ( *t > 0 ) t[1] = 0;
						while ( t < m ) *r1++ = *t++;
						continue;
					}
/*
					Now we have a nontrivial argument that should be studied.
					Try to find common factors.
*/
					lp += 2;
					if ( *t < 0 ) {
						if ( factor && ( *factor == 0 && *t == -SNUMBER ) ) {
							*r1++ = *t++;
							if ( *t == 0 ) *r1++ = *t++;
							else { *r1++ = 1; t++; }
							continue;
						}
						else if ( factor && *factor == 4 && factor[2] == 1 ) {
							if ( *t == -SNUMBER ) {
								if ( factor[3] < 0 || t[1] >= 0 ) {
									while ( t < m ) *r1++ = *t++;
								}
								else {
									*r1++ = -SNUMBER; *r1++ = -1;
									*r1++ = *t++; *r1++ = -*t++;
								}
							}
							else {
								while ( t < m ) *r1++ = *t++;
								*r1++ = -SNUMBER; *r1++ = 1;
							}
							continue;
						}
					}
					r3 = t + *t;
					t += ARGHEAD;  r5 = t; /* Store starting point */
					/* We have terms from r5 to r3 */
					if ( r5+*r5 == r3 && factor ) { /* One term only */
						if ( *factor == 0 ) {
							GETSTOP(t,r6);
							r9 = r1; *r1++ = 0; *r1++ = 1;
							FILLARG(r1);
							*r1++ = (r6-t)+3; t++;
							while ( t < r6 ) *r1++ = *t++;
							*r1++ = 1; *r1++ = 1; *r1++ = 3;
							*r9 = r1-r9;
							if ( ToFast(r9,r9) ) {
								if ( *r9 <= -FUNCTION ) r1 = r9+1;
								else r1 = r9+2;
							}
							t = r3; continue;
						}
						if ( factor[0] == 4 && factor[2] == 1 ) {
							GETSTOP(t,r6);
							r7 = r1; *r1++ = (r6-t)+3+ARGHEAD; *r1++ = 0;
							FILLARG(r1);
							*r1++ = (r6-t)+3; t++;
							while ( t < r6 ) *r1++ = *t++;
							*r1++ = 1; *r1++ = 1; *r1++ = 3;
							if ( ToFast(r7,r7) ) {
								if ( *r7 <= -FUNCTION ) r1 = r7+1;
								else r1 = r7+2;
							}
							if ( r3[-1] < 0 && factor[3] > 0 ) {
								*r1++ = -SNUMBER; *r1++ = -1;
								if ( r3[-1] == -3 && r3[-2] == 1
								&& ( r3[-3] & MAXPOSITIVE ) == r3[-3] ) {
									*r1++ = -SNUMBER; *r1++ = r3[-3];
								}
								else {
									*r1++ = (r3-r6)+1+ARGHEAD;
									*r1++ = 0;
									FILLARG(r1);
									*r1++ = (r3-r6+1);
									while ( t < r3 ) *r1++ = *t++;
									r1[-1] = -r1[-1];
								}
							}
							else {
								if ( ( r3[-1] == -3 || r3[-1] == 3 )
								&& r3[-2] == 1
								&& ( r3[-3] & MAXPOSITIVE ) == r3[-3] ) {
									*r1++ = -SNUMBER; *r1++ = r3[-3];
									if ( r3[-1] < 0 ) r1[-1] = - r1[-1];
								}
								else {
									*r1++ = (r3-r6)+1+ARGHEAD;
									*r1++ = 0;
									FILLARG(r1);
									*r1++ = (r3-r6+1);
									while ( t < r3 ) *r1++ = *t++;
								}
							}
							t = r3; continue;
						}
					}
					rnext = t + *t;
					GETSTOP(t,r6);
					t++;
					if ( factor == 0 ) {
					while ( t < r6 ) {
						if ( *t == SYMBOL ) {
							r7 = t; r8 = t + t[1]; t += 2;
							while ( t < r8 ) {
								pow = t[1];
								mm = rnext;
								while ( mm < r3 ) {
									mnext = mm + *mm;
									GETSTOP(mm,mstop); mm++;
									while ( mm < mstop ) {
										if ( *mm != SYMBOL ) mm += mm[1];
										else break;
									}
									if ( *mm == SYMBOL ) {
										mstop = mm + mm[1]; mm += 2;
										while ( *mm != *t && mm < mstop ) mm += 2;
										if ( mm >= mstop ) pow = 0;
										else if ( pow > 0 && mm[1] > 0 ) {
											if ( mm[1] < pow ) pow = mm[1];
										}
										else if ( pow < 0 && mm[1] < 0 ) {
											if ( mm[1] > pow ) pow = mm[1];
										}
										else pow = 0;
									}
									else pow = 0;
									if ( pow == 0 ) break;
									mm = mnext;
								}
								if ( pow == 0 ) { t += 2; continue; }
/*
								We have a factor
*/
								action = 1; i = pow;
								if ( i > 0 ) {
									while ( --i >= 0 ) {
										*r1++ = -SYMBOL;
										*r1++ = *t;
									}
								}
								else {
									while ( i++ < 0 ) {
										*r1++ = 8 + ARGHEAD;
										for ( j = 1; j < ARGHEAD; j++ ) *r1++ = 0;
										*r1++ = 8; *r1++ = SYMBOL;
										*r1++ = 4; *r1++ = *t; *r1++ = -1;
										*r1++ = 1; *r1++ = 1; *r1++ = 3;
									}
								}
/*
								Now we have to remove the symbols
*/
								t[1] -= pow;
								mm = rnext;
								while ( mm < r3 ) {
									mnext = mm + *mm;
									GETSTOP(mm,mstop); mm++;
									while ( mm < mstop ) {
										if ( *mm != SYMBOL ) mm += mm[1];
										else break;
									}
									mstop = mm + mm[1]; mm += 2;
									while ( mm < mstop && *mm != *t ) mm += 2;
									mm[1] -= pow;
									mm = mnext;
								}
								t += 2;
							}
						}
						else if ( *t == DOTPRODUCT ) {
							r7 = t; r8 = t + t[1]; t += 2;
							while ( t < r8 ) {
								pow = t[2];
								mm = rnext;
								while ( mm < r3 ) {
									mnext = mm + *mm;
									GETSTOP(mm,mstop); mm++;
									while ( mm < mstop ) {
										if ( *mm != DOTPRODUCT ) mm += mm[1];
										else break;
									}
									if ( *mm == DOTPRODUCT ) {
										mstop = mm + mm[1]; mm += 2;
										while ( ( *mm != *t || mm[1] != t[1] )
											 && mm < mstop ) mm += 3;
										if ( mm >= mstop ) pow = 0;
										else if ( pow > 0 && mm[2] > 0 ) {
											if ( mm[2] < pow ) pow = mm[2];
										}
										else if ( pow < 0 && mm[2] < 0 ) {
											if ( mm[2] > pow ) pow = mm[2];
										}
										else pow = 0;
									}
									else pow = 0;
									if ( pow == 0 ) break;
									mm = mnext;
								}
								if ( pow == 0 ) { t += 3; continue; }
/*
								We have a factor
*/
								action = 1; i = pow;
								if ( i > 0 ) {
									while ( --i >= 0 ) {
										*r1++ = 9 + ARGHEAD;
										for ( j = 1; j < ARGHEAD; j++ ) *r1++ = 0;
										*r1++ = 9; *r1++ = DOTPRODUCT;
										*r1++ = 5; *r1++ = *t; *r1++ = t[1]; *r1++ = 1;
										*r1++ = 1; *r1++ = 1; *r1++ = 3;
									}
								}
								else {
									while ( i++ < 0 ) {
										*r1++ = 9 + ARGHEAD;
										for ( j = 1; j < ARGHEAD; j++ ) *r1++ = 0;
										*r1++ = 9; *r1++ = DOTPRODUCT;
										*r1++ = 5; *r1++ = *t; *r1++ = t[1]; *r1++ = -1;
										*r1++ = 1; *r1++ = 1; *r1++ = 3;
									}
								}
/*
								Now we have to remove the dotproducts
*/
								t[2] -= pow;
								mm = rnext;
								while ( mm < r3 ) {
									mnext = mm + *mm;
									GETSTOP(mm,mstop); mm++;
									while ( mm < mstop ) {
										if ( *mm != DOTPRODUCT ) mm += mm[1];
										else break;
									}
									mstop = mm + mm[1]; mm += 2;
									while ( mm < mstop && ( *mm != *t
										|| mm[1] != t[1] ) ) mm += 3;
									mm[2] -= pow;
									mm = mnext;
								}
								t += 3;
							}
						}
						else if ( *t == DELTA || *t == VECTOR ) {
							r7 = t; r8 = t + t[1]; t += 2;
							while ( t < r8 ) {
								mm = rnext;
								pow = 1;
								while ( mm < r3 ) {
									mnext = mm + *mm;
									GETSTOP(mm,mstop); mm++;
									while ( mm < mstop ) {
										if ( *mm != *r7 ) mm += mm[1];
										else break;
									}
									if ( *mm == *r7 ) {
										mstop = mm + mm[1]; mm += 2;
										while ( ( *mm != *t || mm[1] != t[1] )
											&& mm < mstop ) mm += 2;
										if ( mm >= mstop ) pow = 0;
									}
									else pow = 0;
									if ( pow == 0 ) break;
									mm = mnext;
								}
								if ( pow == 0 ) { t += 2; continue; }
/*
								We have a factor
*/
								action = 1;
								*r1++ = 8 + ARGHEAD;
								for ( j = 1; j < ARGHEAD; j++ ) *r1++ = 0;
								*r1++ = 8; *r1++ = *r7;
								*r1++ = 4; *r1++ = *t; *r1++ = t[1];
								*r1++ = 1; *r1++ = 1; *r1++ = 3;
/*
								Now we have to remove the delta's/vectors
*/
								*t = t[1] = NOINDEX;
								mm = rnext;
								while ( mm < r3 ) {
									mnext = mm + *mm;
									GETSTOP(mm,mstop); mm++;
									while ( mm < mstop ) {
										if ( *mm != *r7 ) mm += mm[1];
										else break;
									}
									mstop = mm + mm[1]; mm += 2;
									while ( mm < mstop && (
									 *mm != *t || mm[1] != t[1] ) ) mm += 2;
									*mm = mm[1] = NOINDEX;
									mm = mnext;
								}
								t += 2;
							}
						}
						else if ( *t == INDEX ) {
							r7 = t; r8 = t + t[1]; t += 2;
							while ( t < r8 ) {
								mm = rnext;
								pow = 1;
								while ( mm < r3 ) {
									mnext = mm + *mm;
									GETSTOP(mm,mstop); mm++;
									while ( mm < mstop ) {
										if ( *mm != *r7 ) mm += mm[1];
										else break;
									}
									if ( *mm == *r7 ) {
										mstop = mm + mm[1]; mm += 2;
										while ( *mm != *t 
											&& mm < mstop ) mm++;
										if ( mm >= mstop ) pow = 0;
									}
									else pow = 0;
									if ( pow == 0 ) break;
									mm = mnext;
								}
								if ( pow == 0 ) { t++; continue; }
/*
								We have a factor
*/
								action = 1;
								*r1++ = 7 + ARGHEAD;
								for ( j = 1; j < ARGHEAD; j++ ) *r1++ = 0;
								*r1++ = 7; *r1++ = *r7;
								*r1++ = 3; *r1++ = *t;
								*r1++ = 1; *r1++ = 1; *r1++ = 3;
/*
								Now we have to remove the index
*/
								*t = NOINDEX;
								mm = rnext;
								while ( mm < r3 ) {
									mnext = mm + *mm;
									GETSTOP(mm,mstop); mm++;
									while ( mm < mstop ) {
										if ( *mm != *r7 ) mm += mm[1];
										else break;
									}
									mstop = mm + mm[1]; mm += 2;
									while ( mm < mstop && 
									 *mm != *t ) mm += 1;
									*mm = NOINDEX;
									mm = mnext;
								}
								t += 1;
							}
						}
						else if ( *t >= FUNCTION ) {
/*
							In the next code we should actually look inside
							the DENOMINATOR or EXPONENT for noncommuting objects
*/
							if ( *t >= FUNCTION &&
								functions[*t-FUNCTION].commute == 0 ) ncom = 0;
							else ncom = 1;
							if ( ncom ) {
								mm = r5 + 1;
								while ( mm < t && ( *mm == DUMMYFUN
								|| *mm == DUMMYTEN ) ) mm += mm[1];
								if ( mm < t ) { t += t[1]; continue; }
							}
							mm = rnext; pow = 1;
							while ( mm < r3 ) {
								mnext = mm + *mm;
								GETSTOP(mm,mstop); mm++;
								while ( mm < mstop ) {
									if ( *mm == *t && mm[1] == t[1] ) {
										for ( i = 2; i < t[1]; i++ ) {
											if ( mm[i] != t[i] ) break;
										}
										if ( i >= t[1] )
											{ mm += mm[1]; goto nextmterm; }
									}
									if ( ncom && *mm != DUMMYFUN && *m != DUMMYTEN )
										{ pow = 0; break; }
									mm += mm[1];
								}
								if ( mm >= mstop ) pow = 0;
								if ( pow == 0 ) break;
nextmterm:						mm = mnext;
							}
							if ( pow == 0 ) { t += t[1]; continue; }
/*
							Copy the function
*/
							*r1++ = t[1] + 4 + ARGHEAD;
							for ( i = 1; i < ARGHEAD; i++ ) *r1++ = 0;
							*r1++ = t[1] + 4;
							for ( i = 0; i < t[1]; i++ ) *r1++ = t[i];
							*r1++ = 1; *r1++ = 1; *r1++ = 3;
/*
							Now we have to take out the functions
*/
							mm = rnext;
							while ( mm < r3 ) {
								mnext = mm + *mm;
								GETSTOP(mm,mstop); mm++;
								while ( mm < mstop ) {
									if ( *mm == *t && mm[1] == t[1] ) {
										for ( i = 2; i < t[1]; i++ ) {
											if ( mm[i] != t[i] ) break;
										}
										if ( i >= t[1] ) {
											if ( functions[*t-FUNCTION].spec > 0 )
												*mm = DUMMYTEN;
											else
												*mm = DUMMYFUN;
											mm += mm[1];
											goto nextterm;
										}
									}
									mm += mm[1];
								}
nextterm:						mm = mnext;
							}
							if ( functions[*t-FUNCTION].spec > 0 )
									*t = DUMMYTEN;
							else
									*t = DUMMYFUN;
							t += t[1];
						}
						else {
							t += t[1];
						}
					}
					}
					t = r5; pow = 1;
					while ( t < r3 ) {
						t += *t; if ( t[-1] > 0 ) { pow = 0; break; }
					}
/*
					We have to add here the code for computing the GCD
					and to divide it out.
*/
					t = r5;
					if ( t + *t == r3 ) goto oneterm;
					if ( EAscrat == 0 ) {
						EAscrat = (UWORD *)Malloc1(2*(AM.MaxTal+2)*sizeof(UWORD)
								,"execarg");
					}
					GETSTOP(t,r6);
					ngcd = t[t[0]-1];
					i = abs(ngcd)-1;
					while ( --i >= 0 ) EAscrat[i] = r6[i];
					t += *t;
					while ( t < r3 ) {
						GETSTOP(t,r6);
						i = t[t[0]-1];
						if ( AccumGCD(EAscrat,&ngcd,(UWORD *)r6,i) ) goto execargerr;
						if ( ngcd == 3 && EAscrat[0] == 1 && EAscrat[1] == 1 ) break;
						t += *t;
					}
 					if ( ngcd != 3 || EAscrat[0] != 1 || EAscrat[1] != 1 ) {
						if ( pow ) ngcd = -ngcd;
						t = r5; r9 = r1; *r1++ = t[-ARGHEAD]; *r1++ = 1;
						FILLARG(r1); ngcd = REDLENG(ngcd);
						while ( t < r3 ) {
							GETSTOP(t,r6);
							r7 = t; r8 = r1;
							while ( r7 < r6) *r1++ = *r7++;
							t += *t;
							i = REDLENG(t[-1]);
							if ( DivRat((UWORD *)r6,i,EAscrat,ngcd,(UWORD *)r1,&nq) ) goto execargerr;
							nq = INCLENG(nq);
							i = ABS(nq)-1;
							r1 += i; *r1++ = nq; *r8 = r1-r8;
						}
						*r9 = r1-r9;
						ngcd = INCLENG(ngcd);
						i = ABS(ngcd)-1;
						if ( factor && *factor == 0 ) {}
						else if ( ( factor && factor[0] == 4 && factor[2] == 1
						&& factor[3] == -3 ) || pow == 0 ) {
							r9 = r1; *r1++ = ARGHEAD+2+i; *r1++ = 0;
							FILLARG(r1); *r1++ = i+2;
							for ( j = 0; j < i; j++ ) *r1++ = EAscrat[j];
							*r1++ = ngcd;
							if ( ToFast(r9,r9) ) r1 = r9+2;
						}
						else if ( factor && factor[0] == 4 && factor[2] == 1
						&& factor[3] > 0 && pow ) {
							if ( ngcd < 0 ) ngcd = -ngcd;
							*r1++ = -SNUMBER; *r1++ = -1;
							r9 = r1; *r1++ = ARGHEAD+2+i; *r1++ = 0;
							FILLARG(r1); *r1++ = i+2;
							for ( j = 0; j < i; j++ ) *r1++ = EAscrat[j];
							*r1++ = ngcd;
							if ( ToFast(r9,r9) ) r1 = r9+2;
						}
						else {
							if ( ngcd < 0 ) ngcd = -ngcd;
							if ( pow ) { *r1++ = -SNUMBER; *r1++ = -1; }
							if ( ngcd != 3 || EAscrat[0] != 1 || EAscrat[1] != 1 ) {
								r9 = r1; *r1++ = ARGHEAD+2+i; *r1++ = 0;
								FILLARG(r1); *r1++ = i+2;
								for ( j = 0; j < i; j++ ) *r1++ = EAscrat[j];
								*r1++ = ngcd;
								if ( ToFast(r9,r9) ) r1 = r9+2;
							}
						}
 					}
					else {
oneterm:;
						if ( factor == 0 || *factor > 2 ) {
						if ( pow > 0 ) {
							*r1++ = -SNUMBER; *r1++ = -1;
							t = r5;
							while ( t < r3 ) {
								t += *t; t[-1] = -t[-1];
							}
						}
						t = rr; *r1++ = *t++; *r1++ = 1; t++;
						COPYARG(r1,t);
						while ( t < m ) *r1++ = *t++;
						}
					}
				}
				r2[1] = r1 - r2;
			}
			else {
				r = t + t[1];
				while ( t < r ) *r1++ = *t++;
			}
		}
		r = term + *term;
		while ( t < r ) *r1++ = *t++;
		m = AR.WorkPointer;
		i = m[0] = r1 - m;
		t = term;
		while ( --i >= 0 ) *t++ = *m++;
		if ( AR.WorkPointer < t ) AR.WorkPointer = t;
	}
	C->numlhs = oldnumlhs;
	if ( action && Normalize(term) ) goto execargerr;
	AR.WorkPointer = oldwork;
	if ( AR.WorkPointer < term + *term ) AR.WorkPointer = term + *term;
	AR.pWorkPointer = oldppointer;
	return(action);
execargerr:
	AR.WorkPointer = oldwork;
	AR.pWorkPointer = oldppointer;
	return(MesCall("execarg"));
}

/*
  	#] execarg :
  	#[ execterm :
*/

WORD
execterm ARG2(WORD *,term,WORD,level)
{
	CBUF *C = cbuf+AM.rbufnum;
	WORD oldnumlhs = C->numlhs;
	WORD maxisat = C->lhs[level][2];
	WORD *buffer1 = 0;
	WORD *oldworkpointer = AR.WorkPointer;
	WORD *t1, i;
	do {
		C->numlhs = C->lhs[level][3];
		NewSort();
		if ( buffer1 ) {
			term = buffer1;
			while ( *term ) {
				t1 = oldworkpointer;
				i = *term; while ( --i >= 0 ) *t1++ = *term++;
				AR.WorkPointer = t1;
				if ( Generator(oldworkpointer,level) ) goto exectermerr;
			}
		}
		else {
			if ( Generator(term,level) ) goto exectermerr;
		}
		if ( buffer1 ) {
			M_free((void *)buffer1,"buffer in sort statement");
			buffer1 = 0;
		}
		if ( EndSort((WORD *)(&buffer1),2) < 0 ) goto exectermerr;
		level = C->numlhs;
	} while ( C->numlhs < maxisat );
	C->numlhs = oldnumlhs;
	term = buffer1;
	while ( *term ) {
		t1 = oldworkpointer;
		i = *term; while ( --i >= 0 ) *t1++ = *term++;
		AR.WorkPointer = t1;
		if ( Generator(oldworkpointer,level) ) goto exectermerr;
	}
	M_free(buffer1,"buffer in term statement");
	AR.WorkPointer = oldworkpointer;
	return(0);
exectermerr:
	AR.WorkPointer = oldworkpointer;
	return(MesCall("execterm"));
}

/*
  	#] execterm : 
*/

