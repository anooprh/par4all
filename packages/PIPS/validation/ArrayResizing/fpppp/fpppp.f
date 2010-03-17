*DECK FPPPP
      SUBROUTINE FPPPP
      IMPLICIT REAL*8(A-H,O-Z)
      DIMENSION E(256)
      COMMON /FFQ/ FQ0,FQ1,FQ2,FQ3,FQ4,FQ5
      COMMON /FP4/ QA,QA1,QA2,A12I,A34I,A1234I
      COMMON /FP4/ A1,A2,A3,A4,A12,A34,A1234,PQX,PQY,PQZ,
     *   PQXX,PQYY,PQZZ,PQXY,PQXZ,PQYZ,
     *V0000,V0010,V0020,V0030,V0100,V0200,V0300,
     *V0110,V0120,V0130,V0210,V0220,V0230,V0310,V0320,V0330,
     *V1010,V1020,V1030,V2010,V2020,V2030,V3010,V3020,V3030,
     *V1000,V2000,V3000,V1100,V2100,V3100,V1200,V2200,V3200,
     *V1300,V2300,V3300,V1110,V2110,V3110,V1210,V2210,V3210,
     *V1310,V2310,V3310,V1120,V2120,V3120,V1220,V2220,V3220,
     *V1320,V2320,V3320,V1130,V2130,V3130,V1230,V2230,V3230,
     *V1330,V2330,V3330
      COMMON /FP4/C1110,C2110,C3110,C1210,C2210,C3210,
     *C1320,C2320,C3320,C1130,C2130,C3130,C1230,C2230,C3230,
     *C1310,C2310,C3310,C1120,C2120,C3120,C1220,C2220,C3220,
     *C1330,C2330,C3330,OPXO,OPYO,OPZO,OPOX,OPOY,OPOZ,OPXOX,OPYOY,OPZOZ,
     *OPXX,OPXY,OPXZ,OPYX,OPYY,OPYZ,OPZX,OPZY,OPZZ,OQXO,OQYO,OQZO,
     *OQOX,OQOY,OQOZ,OQXOX,OQYOY,OQZOZ,OQXX,OQXY,OQXZ,OQYX,OQYY,OQYZ,
     *OQZX,OQZY,OQZZ,S1,S2,S3,S4,S12,S34
      COMMON /FP4/ E,
     *GOOOO,GOOXO,GOOYO,GOOZO,GXOOO,GXOXO,GXOYO,GXOZO,GXXOO,GXXXO,GXXYO,
     *GXXZO,GXYOO,GXYZO,GXZOO,GYOOO,GYOYO,GYOZO,GYYOO,GYYXO,GYYYO,
     *GYYZO,GYZOO,GZOOO,GZOZO,GZZOO,GZZXO,GZZYO,GZZZO,
     *VE00,VE11,VE12,VE13,VE14,VE21,VE22,VE23,VE24,VE31,VE32,VE33,VE34,
     *CSSSP,CSSPP,CSPSP,CPSSP,CSPPP,CPSPP,CPPSP,CPPPP
      EQUIVALENCE (GXYOO,GYXOO)
      EQUIVALENCE (GXZOO,GZXOO)
      EQUIVALENCE (GYZOO,GZYOO)
      EQUIVALENCE (GYXXO,GXYXO,GXXYO)
      EQUIVALENCE (GZXXO,GXZXO,GXXZO)
      EQUIVALENCE (GZYYO,GYZYO,GYYZO)
      EQUIVALENCE (GXYYO,GYXYO,GYYXO)
      EQUIVALENCE (GXZZO,GZXZO,GZZXO)
      EQUIVALENCE (GYZZO,GZYZO,GZZYO)
      EQUIVALENCE (GXYZO,GYZXO,GZXYO,GZYXO,GYXZO,GXZYO)
      EQUIVALENCE (GYYXX,GYXYX,GYXXY,GXYYX,GXYXY,GXXYY)
      EQUIVALENCE (GZZXX,GZXZX,GZXXZ,GXZZX,GXZXZ,GXXZZ)
      EQUIVALENCE (GZZYY,GZYZY,GZYYZ,GYZZY,GYZYZ,GYYZZ)
      EQUIVALENCE (GYXXX,GXYXX,GXXYX,GXXXY)
      EQUIVALENCE (GZXXX,GXZXX,GXXZX,GXXXZ)
      EQUIVALENCE (GXYYY,GYXYY,GYYXY,GYYYX)
      EQUIVALENCE (GZYYY,GYZYY,GYYZY,GYYYZ)
      EQUIVALENCE (GXZZZ,GZXZZ,GZZXZ,GZZZX)
      EQUIVALENCE (GYZZZ,GZYZZ,GZZYZ,GZZZY)
      EQUIVALENCE (GXYZZ,GXZYZ,GXZZY,GYXZZ,GYZXZ,GYZZX,
     *             GZXYZ,GZXZY,GZYXZ,GZYZX,GZZXY,GZZYX)
      EQUIVALENCE (GYZXX,GYXZX,GYXXZ,GZYXX,GZXYX,GZXXY,
     *             GXYZX,GXYXZ,GXZYX,GXZXY,GXXYZ,GXXZY)
      EQUIVALENCE (GZXYY,GZYXY,GZYYX,GXZYY,GXYZY,GXYYZ,
     *             GYZXY,GYZYX,GYXZY,GYXYZ,GYYZX,GYYXZ)
C
      DATA THREE,  P25,   H
     $ /    3.D0,.25D0,0.5D0/
C
C
      V0001=OQOX*GOOOO+GOOXO
      V0002=OQOY*GOOOO+GOOYO
      V0003=OQOZ*GOOOO+GOOZO
      VE00=VE00+(V0001*E(  2)+V0002*E(  3)+V0003*E(  4))*CSSSP
      TEMP=V0000*CSSSP
      VE14=TEMP*E(2)
      VE24=TEMP*E(3)
      VE34=TEMP*E(4)
      PTOQ=-A12*A34I
      PTOQS=PTOQ**2
      GOOXX=GXXOO*PTOQS
      GOOYY=GYYOO*PTOQS
      GOOZZ=GZZOO*PTOQS
      GOOXY=GXYOO*PTOQS
      GOOXZ=GXZOO*PTOQS
      GOOYZ=GYZOO*PTOQS
      V0011=OQXX*GOOOO+OQXOX*GOOXO+GOOXX
      V0022=OQYY*GOOOO+OQYOY*GOOYO+GOOYY
      V0033=OQZZ*GOOOO+OQZOZ*GOOZO+GOOZZ
      V0012=OQXO*V0002+OQOY*GOOXO+GOOXY
      V0013=OQXO*V0003+OQOZ*GOOXO+GOOXZ
      V0021=OQYO*V0001+OQOX*GOOYO+GOOXY
      V0023=OQYO*V0003+OQOZ*GOOYO+GOOYZ
      V0031=OQZO*V0001+OQOX*GOOZO+GOOXZ
      V0032=OQZO*V0002+OQOY*GOOZO+GOOYZ
      VE00=VE00+(V0011*E(  6)+V0012*E(  7)+V0013*E(  8)
     *          +V0021*E( 10)+V0022*E( 11)+V0023*E( 12)
     *          +V0031*E( 14)+V0032*E( 15)+V0033*E( 16))*CSSPP
      VE14=VE14+(V0010*E(6)+V0020*E(10)+V0030*E(14))*CSSPP
      VE24=VE24+(V0010*E(7)+V0020*E(11)+V0030*E(15))*CSSPP
      VE34=VE34+(V0010*E(8)+V0020*E(12)+V0030*E(16))*CSSPP
      VE13=VE13+(V0001*E(6)+V0002*E(7)+V0003*E(8))*CSSPP
      VE23=VE23+(V0001*E(10)+V0002*E(11)+V0003*E(12))*CSSPP
      VE33=VE33+(V0001*E(14)+V0002*E(15)+V0003*E(16))*CSSPP
      V0101=OQOX*V0100+OPOX*GOOXO+GXOXO
      V0102=OQOY*V0100+OPOX*GOOYO+GXOYO
      V0103=OQOZ*V0100+OPOX*GOOZO+GXOZO
      V0201=OQOX*V0200+OPOY*GOOXO+GXOYO
      V0202=OQOY*V0200+OPOY*GOOYO+GYOYO
      V0203=OQOZ*V0200+OPOY*GOOZO+GYOZO
      V0301=OQOX*V0300+OPOZ*GOOXO+GXOZO
      V0302=OQOY*V0300+OPOZ*GOOYO+GYOZO
      V0303=OQOZ*V0300+OPOZ*GOOZO+GZOZO
      VE00=VE00+(V0101*E( 18)+V0102*E( 19)+V0103*E( 20)
     *          +V0201*E( 34)+V0202*E( 35)+V0203*E( 36)
     *          +V0301*E( 50)+V0302*E( 51)+V0303*E( 52))*CSPSP
      VE14=VE14+(V0100*E(18)+V0200*E(34)+V0300*E(50))*CSPSP
      VE24=VE24+(V0100*E(19)+V0200*E(35)+V0300*E(51))*CSPSP
      VE34=VE34+(V0100*E(20)+V0200*E(36)+V0300*E(52))*CSPSP
      VE12=VE12+(V0001*E(18)+V0002*E(19)+V0003*E(20))*CSPSP
      VE22=VE22+(V0001*E(34)+V0002*E(35)+V0003*E(36))*CSPSP
      VE32=VE32+(V0001*E(50)+V0002*E(51)+V0003*E(52))*CSPSP
      V1001=OQOX*V1000+OPXO*GOOXO+GXOXO
      V1002=OQOY*V1000+OPXO*GOOYO+GXOYO
      V1003=OQOZ*V1000+OPXO*GOOZO+GXOZO
      V2001=OQOX*V2000+OPYO*GOOXO+GXOYO
      V2002=OQOY*V2000+OPYO*GOOYO+GYOYO
      V2003=OQOZ*V2000+OPYO*GOOZO+GYOZO
      V3001=OQOX*V3000+OPZO*GOOXO+GXOZO
      V3002=OQOY*V3000+OPZO*GOOYO+GYOZO
      V3003=OQOZ*V3000+OPZO*GOOZO+GZOZO
      VE00=VE00+(V1001*E( 66)+V1002*E( 67)+V1003*E( 68)
     *          +V2001*E(130)+V2002*E(131)+V2003*E(132)
     *          +V3001*E(194)+V3002*E(195)+V3003*E(196))*CPSSP
      VE14=VE14+(V1000*E(66)+V2000*E(130)+V3000*E(194))*CPSSP
      VE24=VE24+(V1000*E(67)+V2000*E(131)+V3000*E(195))*CPSSP
      VE34=VE34+(V1000*E(68)+V2000*E(132)+V3000*E(196))*CPSSP
      VE11=VE11+(V0001*E(66)+V0002*E(67)+V0003*E(68))*CPSSP
      VE21=VE21+(V0001*E(130)+V0002*E(131)+V0003*E(132))*CPSSP
      VE31=VE31+(V0001*E(194)+V0002*E(195)+V0003*E(196))*CPSSP
      V1101=OQOX*V1100+C1110
      V1102=OQOY*V1100+C1120
      V1103=OQOZ*V1100+C1130
      V1201=OQOX*V1200+C1210
      V1202=OQOY*V1200+C1220
      V1203=OQOZ*V1200+C1230
      V1301=OQOX*V1300+C1310
      V1302=OQOY*V1300+C1320
      V1303=OQOZ*V1300+C1330
      V2101=OQOX*V2100+C2110
      V2102=OQOY*V2100+C2120
      V2103=OQOZ*V2100+C2130
      V2201=OQOX*V2200+C2210
      V2202=OQOY*V2200+C2220
      V2203=OQOZ*V2200+C2230
      V2301=OQOX*V2300+C2310
      V2302=OQOY*V2300+C2320
      V2303=OQOZ*V2300+C2330
      V3101=OQOX*V3100+C3110
      V3102=OQOY*V3100+C3120
      V3103=OQOZ*V3100+C3130
      V3201=OQOX*V3200+C3210
      V3202=OQOY*V3200+C3220
      V3203=OQOZ*V3200+C3230
      V3301=OQOX*V3300+C3310
      V3302=OQOY*V3300+C3320
      V3303=OQOZ*V3300+C3330
      VE00=VE00+(V1101*E( 82)+V1102*E( 83)+V1103*E( 84)
     *          +V1201*E( 98)+V1202*E( 99)+V1203*E(100)
     *          +V1301*E(114)+V1302*E(115)+V1303*E(116)
     *          +V2101*E(146)+V2102*E(147)+V2103*E(148)
     *          +V2201*E(162)+V2202*E(163)+V2203*E(164)
     *          +V2301*E(178)+V2302*E(179)+V2303*E(180)
     *          +V3101*E(210)+V3102*E(211)+V3103*E(212)
     *          +V3201*E(226)+V3202*E(227)+V3203*E(228)
     *          +V3301*E(242)+V3302*E(243)+V3303*E(244))*CPPSP
      VE14=VE14+(V1100*E( 82)+V1200*E( 98)+V1300*E(114)
     *          +V2100*E(146)+V2200*E(162)+V2300*E(178)
     *          +V3100*E(210)+V3200*E(226)+V3300*E(242))*CPPSP
      VE24=VE24+(V1100*E( 83)+V1200*E( 99)+V1300*E(115)
     *          +V2100*E(147)+V2200*E(163)+V2300*E(179)
     *          +V3100*E(211)+V3200*E(227)+V3300*E(243))*CPPSP
      VE34=VE34+(V1100*E( 84)+V1200*E(100)+V1300*E(116)
     *          +V2100*E(148)+V2200*E(164)+V2300*E(180)
     *          +V3100*E(212)+V3200*E(228)+V3300*E(244))*CPPSP
      VE12=VE12+(V1001*E( 82)+V1002*E( 83)+V1003*E( 84)
     *          +V2001*E(146)+V2002*E(147)+V2003*E(148)
     *          +V3001*E(210)+V3002*E(211)+V3003*E(212))*CPPSP
      VE22=VE22+(V1001*E( 98)+V1002*E( 99)+V1003*E(100)
     *          +V2001*E(162)+V2002*E(163)+V2003*E(164)
     *          +V3001*E(226)+V3002*E(227)+V3003*E(228))*CPPSP
      VE32=VE32+(V1001*E(114)+V1002*E(115)+V1003*E(116)
     *          +V2001*E(178)+V2002*E(179)+V2003*E(180)
     *          +V3001*E(242)+V3002*E(243)+V3003*E(244))*CPPSP
      VE11=VE11+(V0101*E( 82)+V0102*E( 83)+V0103*E( 84)
     *          +V0201*E( 98)+V0202*E( 99)+V0203*E(100)
     *          +V0301*E(114)+V0302*E(115)+V0303*E(116))*CPPSP
      VE21=VE21+(V0101*E(146)+V0102*E(147)+V0103*E(148)
     *          +V0201*E(162)+V0202*E(163)+V0203*E(164)
     *          +V0301*E(178)+V0302*E(179)+V0303*E(180))*CPPSP
      VE31=VE31+(V0101*E(210)+V0102*E(211)+V0103*E(212)
     *          +V0201*E(226)+V0202*E(227)+V0203*E(228)
     *          +V0301*E(242)+V0302*E(243)+V0303*E(244))*CPPSP
      GXOYZ=GXYZO*PTOQ
      GXOXX=GXXXO*PTOQ
      GYOYY=GYYYO*PTOQ
      GZOZZ=GZZZO*PTOQ
      GYOXX=GXXYO*PTOQ
      GZOXX=GXXZO*PTOQ
      GXOYY=GYYXO*PTOQ
      GZOYY=GYYZO*PTOQ
      GXOZZ=GZZXO*PTOQ
      GYOZZ=GZZYO*PTOQ
      C1011=OQXX*GXOOO+OQXOX*GXOXO+GXOXX
      C1022=OQYY*GXOOO+OQYOY*GXOYO+GXOYY
      C1033=OQZZ*GXOOO+OQZOZ*GXOZO+GXOZZ
      C2011=OQXX*GYOOO+OQXOX*GXOYO+GYOXX
      C2022=OQYY*GYOOO+OQYOY*GYOYO+GYOYY
      C2033=OQZZ*GYOOO+OQZOZ*GYOZO+GYOZZ
      C3011=OQXX*GZOOO+OQXOX*GXOZO+GZOXX
      C3022=OQYY*GZOOO+OQYOY*GYOZO+GZOYY
      C3033=OQZZ*GZOOO+OQZOZ*GZOZO+GZOZZ
      C1012=OQXY*GXOOO+OQXO*GXOYO+OQOY*GXOXO+GYOXX
      C1013=OQXZ*GXOOO+OQXO*GXOZO+OQOZ*GXOXO+GZOXX
      C1021=OQYX*GXOOO+OQYO*GXOXO+OQOX*GXOYO+GYOXX
      C1023=OQYZ*GXOOO+OQYO*GXOZO+OQOZ*GXOYO+GXOYZ
      C1031=OQZX*GXOOO+OQZO*GXOXO+OQOX*GXOZO+GZOXX
      C1032=OQZY*GXOOO+OQZO*GXOYO+OQOY*GXOZO+GXOYZ
      C2012=OQXY*GYOOO+OQXO*GYOYO+OQOY*GXOYO+GXOYY
      C2013=OQXZ*GYOOO+OQXO*GYOZO+OQOZ*GXOYO+GXOYZ
      C2021=OQYX*GYOOO+OQYO*GXOYO+OQOX*GYOYO+GXOYY
      C2023=OQYZ*GYOOO+OQYO*GYOZO+OQOZ*GYOYO+GZOYY
      C2031=OQZX*GYOOO+OQZO*GXOYO+OQOX*GYOZO+GXOYZ
      C2032=OQZY*GYOOO+OQZO*GYOYO+OQOY*GYOZO+GZOYY
      C3012=OQXY*GZOOO+OQXO*GYOZO+OQOY*GXOZO+GXOYZ
      C3013=OQXZ*GZOOO+OQXO*GZOZO+OQOZ*GXOZO+GXOZZ
      C3021=OQYX*GZOOO+OQYO*GXOZO+OQOX*GYOZO+GXOYZ
      C3023=OQYZ*GZOOO+OQYO*GZOZO+OQOZ*GYOZO+GYOZZ
      C3031=OQZX*GZOOO+OQZO*GXOZO+OQOX*GZOZO+GXOZZ
      C3032=OQZY*GZOOO+OQZO*GYOZO+OQOY*GZOZO+GYOZZ
      V0111=OPOX*V0011+C1011
      V0112=OPOX*V0012+C1012
      V0113=OPOX*V0013+C1013
      V0121=OPOX*V0021+C1021
      V0122=OPOX*V0022+C1022
      V0123=OPOX*V0023+C1023
      V0131=OPOX*V0031+C1031
      V0132=OPOX*V0032+C1032
      V0133=OPOX*V0033+C1033
      V0211=OPOY*V0011+C2011
      V0212=OPOY*V0012+C2012
      V0213=OPOY*V0013+C2013
      V0221=OPOY*V0021+C2021
      V0222=OPOY*V0022+C2022
      V0223=OPOY*V0023+C2023
      V0231=OPOY*V0031+C2031
      V0232=OPOY*V0032+C2032
      V0233=OPOY*V0033+C2033
      V0311=OPOZ*V0011+C3011
      V0312=OPOZ*V0012+C3012
      V0313=OPOZ*V0013+C3013
      V0321=OPOZ*V0021+C3021
      V0322=OPOZ*V0022+C3022
      V0323=OPOZ*V0023+C3023
      V0331=OPOZ*V0031+C3031
      V0332=OPOZ*V0032+C3032
      V0333=OPOZ*V0033+C3033
      VE00=VE00+(V0111*E( 22)+V0112*E( 23)+V0113*E( 24)
     *          +V0121*E( 26)+V0122*E( 27)+V0123*E( 28)
     *          +V0131*E( 30)+V0132*E( 31)+V0133*E( 32)
     *          +V0211*E( 38)+V0212*E( 39)+V0213*E( 40)
     *          +V0221*E( 42)+V0222*E( 43)+V0223*E( 44)
     *          +V0231*E( 46)+V0232*E( 47)+V0233*E( 48)
     *          +V0311*E( 54)+V0312*E( 55)+V0313*E( 56)
     *          +V0321*E( 58)+V0322*E( 59)+V0323*E( 60)
     *          +V0331*E( 62)+V0332*E( 63)+V0333*E( 64))*CSPPP
      VE14=VE14+(V0110*E( 22)+V0120*E( 26)+V0130*E( 30)
     *          +V0210*E( 38)+V0220*E( 42)+V0230*E( 46)
     *          +V0310*E( 54)+V0320*E( 58)+V0330*E( 62))*CSPPP
      VE24=VE24+(V0110*E( 23)+V0120*E( 27)+V0130*E( 31)
     *          +V0210*E( 39)+V0220*E( 43)+V0230*E( 47)
     *          +V0310*E( 55)+V0320*E( 59)+V0330*E( 63))*CSPPP
      VE34=VE34+(V0110*E( 24)+V0120*E( 28)+V0130*E( 32)
     *          +V0210*E( 40)+V0220*E( 44)+V0230*E( 48)
     *          +V0310*E( 56)+V0320*E( 60)+V0330*E( 64))*CSPPP
      VE13=VE13+(V0101*E( 22)+V0102*E( 23)+V0103*E( 24)
     *          +V0201*E( 38)+V0202*E( 39)+V0203*E( 40)
     *          +V0301*E( 54)+V0302*E( 55)+V0303*E( 56))*CSPPP
      VE23=VE23+(V0101*E( 26)+V0102*E( 27)+V0103*E( 28)
     *          +V0201*E( 42)+V0202*E( 43)+V0203*E( 44)
     *          +V0301*E( 58)+V0302*E( 59)+V0303*E( 60))*CSPPP
      VE33=VE33+(V0101*E( 30)+V0102*E( 31)+V0103*E( 32)
     *          +V0201*E( 46)+V0202*E( 47)+V0203*E( 48)
     *          +V0301*E( 62)+V0302*E( 63)+V0303*E( 64))*CSPPP
      VE12=VE12+(V0011*E( 22)+V0012*E( 23)+V0013*E( 24)
     *          +V0021*E( 26)+V0022*E( 27)+V0023*E( 28)
     *          +V0031*E( 30)+V0032*E( 31)+V0033*E( 32))*CSPPP
      VE22=VE22+(V0011*E( 38)+V0012*E( 39)+V0013*E( 40)
     *          +V0021*E( 42)+V0022*E( 43)+V0023*E( 44)
     *          +V0031*E( 46)+V0032*E( 47)+V0033*E( 48))*CSPPP
      VE32=VE32+(V0011*E( 54)+V0012*E( 55)+V0013*E( 56)
     *          +V0021*E( 58)+V0022*E( 59)+V0023*E( 60)
     *          +V0031*E( 62)+V0032*E( 63)+V0033*E( 64))*CSPPP
      V1011=OPXO*V0011+C1011
      V1012=OPXO*V0012+C1012
      V1013=OPXO*V0013+C1013
      V1021=OPXO*V0021+C1021
      V1022=OPXO*V0022+C1022
      V1023=OPXO*V0023+C1023
      V1031=OPXO*V0031+C1031
      V1032=OPXO*V0032+C1032
      V1033=OPXO*V0033+C1033
      V2011=OPYO*V0011+C2011
      V2012=OPYO*V0012+C2012
      V2013=OPYO*V0013+C2013
      V2021=OPYO*V0021+C2021
      V2022=OPYO*V0022+C2022
      V2023=OPYO*V0023+C2023
      V2031=OPYO*V0031+C2031
      V2032=OPYO*V0032+C2032
      V2033=OPYO*V0033+C2033
      V3011=OPZO*V0011+C3011
      V3012=OPZO*V0012+C3012
      V3013=OPZO*V0013+C3013
      V3021=OPZO*V0021+C3021
      V3022=OPZO*V0022+C3022
      V3023=OPZO*V0023+C3023
      V3031=OPZO*V0031+C3031
      V3032=OPZO*V0032+C3032
      V3033=OPZO*V0033+C3033
      VE00=VE00+(V1011*E( 70)+V1012*E( 71)+V1013*E( 72)
     *          +V1021*E( 74)+V1022*E( 75)+V1023*E( 76)
     *          +V1031*E( 78)+V1032*E( 79)+V1033*E( 80)
     *          +V2011*E(134)+V2012*E(135)+V2013*E(136)
     *          +V2021*E(138)+V2022*E(139)+V2023*E(140)
     *          +V2031*E(142)+V2032*E(143)+V2033*E(144)
     *          +V3011*E(198)+V3012*E(199)+V3013*E(200)
     *          +V3021*E(202)+V3022*E(203)+V3023*E(204)
     *          +V3031*E(206)+V3032*E(207)+V3033*E(208))*CPSPP
      VE14=VE14+(V1010*E( 70)+V1020*E( 74)+V1030*E( 78)
     *          +V2010*E(134)+V2020*E(138)+V2030*E(142)
     *          +V3010*E(198)+V3020*E(202)+V3030*E(206))*CPSPP
      VE24=VE24+(V1010*E( 71)+V1020*E( 75)+V1030*E( 79)
     *          +V2010*E(135)+V2020*E(139)+V2030*E(143)
     *          +V3010*E(199)+V3020*E(203)+V3030*E(207))*CPSPP
      VE34=VE34+(V1010*E( 72)+V1020*E( 76)+V1030*E( 80)
     *          +V2010*E(136)+V2020*E(140)+V2030*E(144)
     *          +V3010*E(200)+V3020*E(204)+V3030*E(208))*CPSPP
      VE13=VE13+(V1001*E( 70)+V1002*E( 71)+V1003*E( 72)
     *          +V2001*E(134)+V2002*E(135)+V2003*E(136)
     *          +V3001*E(198)+V3002*E(199)+V3003*E(200))*CPSPP
      VE23=VE23+(V1001*E( 74)+V1002*E( 75)+V1003*E( 76)
     *          +V2001*E(138)+V2002*E(139)+V2003*E(140)
     *          +V3001*E(202)+V3002*E(203)+V3003*E(204))*CPSPP
      VE33=VE33+(V1001*E( 78)+V1002*E( 79)+V1003*E( 80)
     *          +V2001*E(142)+V2002*E(143)+V2003*E(144)
     *          +V3001*E(206)+V3002*E(207)+V3003*E(208))*CPSPP
      VE11=VE11+(V0011*E( 70)+V0012*E( 71)+V0013*E( 72)
     *          +V0021*E( 74)+V0022*E( 75)+V0023*E( 76)
     *          +V0031*E( 78)+V0032*E( 79)+V0033*E( 80))*CPSPP
      VE21=VE21+(V0011*E(134)+V0012*E(135)+V0013*E(136)
     *          +V0021*E(138)+V0022*E(139)+V0023*E(140)
     *          +V0031*E(142)+V0032*E(143)+V0033*E(144))*CPSPP
      VE31=VE31+(V0011*E(198)+V0012*E(199)+V0013*E(200)
     *          +V0021*E(202)+V0022*E(203)+V0023*E(204)
     *          +V0031*E(206)+V0032*E(207)+V0033*E(208))*CPSPP
      QFQ4=(QA1*QA2)**2*FQ4
      TEMP=A1234I**2
      HFQ3=H*QA*FQ3*TEMP
      TFQ3=THREE*HFQ3
      SFQ3=TFQ3+TFQ3
      P25FQ2=P25*FQ2*TEMP
      P75FQ2=THREE*P25FQ2
      TEMP=PQXX*QFQ4
      GXXXX=PQXX*(TEMP-SFQ3)+P75FQ2
      GYXXX=PQXY*(TEMP-TFQ3)
      GZXXX=PQXZ*(TEMP-TFQ3)
      GZYXX=PQYZ*(TEMP-HFQ3)
      GYYXX=PQYY*(TEMP-HFQ3)-PQXX*HFQ3+P25FQ2
      TEMP=PQYY*QFQ4
      GYYYY=PQYY*(TEMP-SFQ3)+P75FQ2
      GXYYY=PQXY*(TEMP-TFQ3)
      GZXYY=PQXZ*(TEMP-HFQ3)
      GZYYY=PQYZ*(TEMP-TFQ3)
      GZZYY=PQZZ*(TEMP-HFQ3)-PQYY*HFQ3+P25FQ2
      TEMP=PQZZ*QFQ4
      GZZZZ=PQZZ*(TEMP-SFQ3)+P75FQ2
      GXYZZ=PQXY*(TEMP-HFQ3)
      GXZZZ=PQXZ*(TEMP-TFQ3)
      GYZZZ=PQYZ*(TEMP-TFQ3)
      GXXZZ=PQXX*(TEMP-HFQ3)-PQZZ*HFQ3+P25FQ2
C      1111 
      VP4=(OPXX*V0011+OPXOX*C1011+OQXX*GXXOO
     1+OQXOX*GXXXO+GXXXX)*E( 86)
C      1112 
      VP4=(OPXX*V0012+OPXOX*C1012+OQXY*GXXOO
     1+OQXO*GXXYO+OQOY*GXXXO+GXXXY)*E( 87)+VP4
C      1113 
      VP4=(OPXX*V0013+OPXOX*C1013+OQXZ*GXXOO
     1+OQXO*GXXZO+OQOZ*GXXXO+GXXXZ)*E( 88)+VP4
C      1121 
      VP4=(OPXX*V0021+OPXOX*C1021+OQYX*GXXOO
     1+OQYO*GXXXO+OQOX*GXXYO+GXXYX)*E( 90)+VP4
C      1122 
      VP4=(OPXX*V0022+OPXOX*C1022+OQYY*GXXOO
     1+OQYOY*GXXYO+GXXYY)*E( 91)+VP4
C      1123 
      VP4=(OPXX*V0023+OPXOX*C1023+OQYZ*GXXOO
     1+OQYO*GXXZO+OQOZ*GXXYO+GXXYZ)*E( 92)+VP4
C      1131 
      VP4=(OPXX*V0031+OPXOX*C1031+OQZX*GXXOO
     1+OQZO*GXXXO+OQOX*GXXZO+GXXZX)*E( 94)+VP4
C      1132 
      VP4=(OPXX*V0032+OPXOX*C1032+OQZY*GXXOO
     1+OQZO*GXXYO+OQOY*GXXZO+GXXZY)*E( 95)+VP4
C      1133 
      VP4=(OPXX*V0033+OPXOX*C1033+OQZZ*GXXOO
     1+OQZOZ*GXXZO+GXXZZ)*E( 96)+VP4
C      1211 
      VP4=(OPXY*V0011+OPXO*C2011+OPOY*C1011+OQXX*GXYOO
     1+OQXOX*GXYXO+GXYXX)*E(102)+VP4
C      1212 
      VP4=(OPXY*V0012+OPXO*C2012+OPOY*C1012+OQXY*GXYOO
     1+OQXO*GXYYO+OQOY*GXYXO+GXYXY)*E(103)+VP4
C      1213 
      VP4=(OPXY*V0013+OPXO*C2013+OPOY*C1013+OQXZ*GXYOO
     1+OQXO*GXYZO+OQOZ*GXYXO+GXYXZ)*E(104)+VP4
C      1221 
      VP4=(OPXY*V0021+OPXO*C2021+OPOY*C1021+OQYX*GXYOO
     1+OQYO*GXYXO+OQOX*GXYYO+GXYYX)*E(106)+VP4
C      1222 
      VP4=(OPXY*V0022+OPXO*C2022+OPOY*C1022+OQYY*GXYOO
     1+OQYOY*GXYYO+GXYYY)*E(107)+VP4
C      1223 
      VP4=(OPXY*V0023+OPXO*C2023+OPOY*C1023+OQYZ*GXYOO
     1+OQYO*GXYZO+OQOZ*GXYYO+GXYYZ)*E(108)+VP4
C      1231 
      VP4=(OPXY*V0031+OPXO*C2031+OPOY*C1031+OQZX*GXYOO
     1+OQZO*GXYXO+OQOX*GXYZO+GXYZX)*E(110)+VP4
C      1232 
      VP4=(OPXY*V0032+OPXO*C2032+OPOY*C1032+OQZY*GXYOO
     1+OQZO*GXYYO+OQOY*GXYZO+GXYZY)*E(111)+VP4
C      1233 
      VP4=(OPXY*V0033+OPXO*C2033+OPOY*C1033+OQZZ*GXYOO
     1+OQZOZ*GXYZO+GXYZZ)*E(112)+VP4
C      1311 
      VP4=(OPXZ*V0011+OPXO*C3011+OPOZ*C1011+OQXX*GXZOO
     1+OQXOX*GXZXO+GXZXX)*E(118)+VP4
C      1312 
      VP4=(OPXZ*V0012+OPXO*C3012+OPOZ*C1012+OQXY*GXZOO
     1+OQXO*GXZYO+OQOY*GXZXO+GXZXY)*E(119)+VP4
C      1313 
      VP4=(OPXZ*V0013+OPXO*C3013+OPOZ*C1013+OQXZ*GXZOO
     1+OQXO*GXZZO+OQOZ*GXZXO+GXZXZ)*E(120)+VP4
C      1321 
      VP4=(OPXZ*V0021+OPXO*C3021+OPOZ*C1021+OQYX*GXZOO
     1+OQYO*GXZXO+OQOX*GXZYO+GXZYX)*E(122)+VP4
C      1322 
      VP4=(OPXZ*V0022+OPXO*C3022+OPOZ*C1022+OQYY*GXZOO
     1+OQYOY*GXZYO+GXZYY)*E(123)+VP4
C      1323 
      VP4=(OPXZ*V0023+OPXO*C3023+OPOZ*C1023+OQYZ*GXZOO
     1+OQYO*GXZZO+OQOZ*GXZYO+GXZYZ)*E(124)+VP4
C      1331 
      VP4=(OPXZ*V0031+OPXO*C3031+OPOZ*C1031+OQZX*GXZOO
     1+OQZO*GXZXO+OQOX*GXZZO+GXZZX)*E(126)+VP4
C      1332 
      VP4=(OPXZ*V0032+OPXO*C3032+OPOZ*C1032+OQZY*GXZOO
     1+OQZO*GXZYO+OQOY*GXZZO+GXZZY)*E(127)+VP4
C      1333 
      VP4=(OPXZ*V0033+OPXO*C3033+OPOZ*C1033+OQZZ*GXZOO
     1+OQZOZ*GXZZO+GXZZZ)*E(128)+VP4
C      2111 
      VP4=(OPYX*V0011+OPYO*C1011+OPOX*C2011+OQXX*GYXOO
     1+OQXOX*GYXXO+GYXXX)*E(150)+VP4
C      2112 
      VP4=(OPYX*V0012+OPYO*C1012+OPOX*C2012+OQXY*GYXOO
     1+OQXO*GYXYO+OQOY*GYXXO+GYXXY)*E(151)+VP4
C      2113 
      VP4=(OPYX*V0013+OPYO*C1013+OPOX*C2013+OQXZ*GYXOO
     1+OQXO*GYXZO+OQOZ*GYXXO+GYXXZ)*E(152)+VP4
C      2121 
      VP4=(OPYX*V0021+OPYO*C1021+OPOX*C2021+OQYX*GYXOO
     1+OQYO*GYXXO+OQOX*GYXYO+GYXYX)*E(154)+VP4
C      2122 
      VP4=(OPYX*V0022+OPYO*C1022+OPOX*C2022+OQYY*GYXOO
     1+OQYOY*GYXYO+GYXYY)*E(155)+VP4
C      2123 
      VP4=(OPYX*V0023+OPYO*C1023+OPOX*C2023+OQYZ*GYXOO
     1+OQYO*GYXZO+OQOZ*GYXYO+GYXYZ)*E(156)+VP4
C      2131 
      VP4=(OPYX*V0031+OPYO*C1031+OPOX*C2031+OQZX*GYXOO
     1+OQZO*GYXXO+OQOX*GYXZO+GYXZX)*E(158)+VP4
C      2132 
      VP4=(OPYX*V0032+OPYO*C1032+OPOX*C2032+OQZY*GYXOO
     1+OQZO*GYXYO+OQOY*GYXZO+GYXZY)*E(159)+VP4
C      2133 
      VP4=(OPYX*V0033+OPYO*C1033+OPOX*C2033+OQZZ*GYXOO
     1+OQZOZ*GYXZO+GYXZZ)*E(160)+VP4
C      2211 
      VP4=(OPYY*V0011+OPYOY*C2011+OQXX*GYYOO
     1+OQXOX*GYYXO+GYYXX)*E(166)+VP4
C      2212 
      VP4=(OPYY*V0012+OPYOY*C2012+OQXY*GYYOO
     1+OQXO*GYYYO+OQOY*GYYXO+GYYXY)*E(167)+VP4
C      2213 
      VP4=(OPYY*V0013+OPYOY*C2013+OQXZ*GYYOO
     1+OQXO*GYYZO+OQOZ*GYYXO+GYYXZ)*E(168)+VP4
C      2221 
      VP4=(OPYY*V0021+OPYOY*C2021+OQYX*GYYOO
     1+OQYO*GYYXO+OQOX*GYYYO+GYYYX)*E(170)+VP4
C      2222 
      VP4=(OPYY*V0022+OPYOY*C2022+OQYY*GYYOO
     1+OQYOY*GYYYO+GYYYY)*E(171)+VP4
C      2223 
      VP4=(OPYY*V0023+OPYOY*C2023+OQYZ*GYYOO
     1+OQYO*GYYZO+OQOZ*GYYYO+GYYYZ)*E(172)+VP4
C      2231 
      VP4=(OPYY*V0031+OPYOY*C2031+OQZX*GYYOO
     1+OQZO*GYYXO+OQOX*GYYZO+GYYZX)*E(174)+VP4
C      2232 
      VP4=(OPYY*V0032+OPYOY*C2032+OQZY*GYYOO
     1+OQZO*GYYYO+OQOY*GYYZO+GYYZY)*E(175)+VP4
C      2233 
      VP4=(OPYY*V0033+OPYOY*C2033+OQZZ*GYYOO
     1+OQZOZ*GYYZO+GYYZZ)*E(176)+VP4
C      2311 
      VP4=(OPYZ*V0011+OPYO*C3011+OPOZ*C2011+OQXX*GYZOO
     1+OQXOX*GYZXO+GYZXX)*E(182)+VP4
C      2312 
      VP4=(OPYZ*V0012+OPYO*C3012+OPOZ*C2012+OQXY*GYZOO
     1+OQXO*GYZYO+OQOY*GYZXO+GYZXY)*E(183)+VP4
C      2313 
      VP4=(OPYZ*V0013+OPYO*C3013+OPOZ*C2013+OQXZ*GYZOO
     1+OQXO*GYZZO+OQOZ*GYZXO+GYZXZ)*E(184)+VP4
C      2321 
      VP4=(OPYZ*V0021+OPYO*C3021+OPOZ*C2021+OQYX*GYZOO
     1+OQYO*GYZXO+OQOX*GYZYO+GYZYX)*E(186)+VP4
C      2322 
      VP4=(OPYZ*V0022+OPYO*C3022+OPOZ*C2022+OQYY*GYZOO
     1+OQYOY*GYZYO+GYZYY)*E(187)+VP4
C      2323 
      VP4=(OPYZ*V0023+OPYO*C3023+OPOZ*C2023+OQYZ*GYZOO
     1+OQYO*GYZZO+OQOZ*GYZYO+GYZYZ)*E(188)+VP4
C      2331 
      VP4=(OPYZ*V0031+OPYO*C3031+OPOZ*C2031+OQZX*GYZOO
     1+OQZO*GYZXO+OQOX*GYZZO+GYZZX)*E(190)+VP4
C      2332 
      VP4=(OPYZ*V0032+OPYO*C3032+OPOZ*C2032+OQZY*GYZOO
     1+OQZO*GYZYO+OQOY*GYZZO+GYZZY)*E(191)+VP4
C      2333 
      VP4=(OPYZ*V0033+OPYO*C3033+OPOZ*C2033+OQZZ*GYZOO
     1+OQZOZ*GYZZO+GYZZZ)*E(192)+VP4
C      3111 
      VP4=(OPZX*V0011+OPZO*C1011+OPOX*C3011+OQXX*GZXOO
     1+OQXOX*GZXXO+GZXXX)*E(214)+VP4
C      3112 
      VP4=(OPZX*V0012+OPZO*C1012+OPOX*C3012+OQXY*GZXOO
     1+OQXO*GZXYO+OQOY*GZXXO+GZXXY)*E(215)+VP4
C      3113 
      VP4=(OPZX*V0013+OPZO*C1013+OPOX*C3013+OQXZ*GZXOO
     1+OQXO*GZXZO+OQOZ*GZXXO+GZXXZ)*E(216)+VP4
C      3121 
      VP4=(OPZX*V0021+OPZO*C1021+OPOX*C3021+OQYX*GZXOO
     1+OQYO*GZXXO+OQOX*GZXYO+GZXYX)*E(218)+VP4
C      3122 
      VP4=(OPZX*V0022+OPZO*C1022+OPOX*C3022+OQYY*GZXOO
     1+OQYOY*GZXYO+GZXYY)*E(219)+VP4
C      3123 
      VP4=(OPZX*V0023+OPZO*C1023+OPOX*C3023+OQYZ*GZXOO
     1+OQYO*GZXZO+OQOZ*GZXYO+GZXYZ)*E(220)+VP4
C      3131 
      VP4=(OPZX*V0031+OPZO*C1031+OPOX*C3031+OQZX*GZXOO
     1+OQZO*GZXXO+OQOX*GZXZO+GZXZX)*E(222)+VP4
C      3132 
      VP4=(OPZX*V0032+OPZO*C1032+OPOX*C3032+OQZY*GZXOO
     1+OQZO*GZXYO+OQOY*GZXZO+GZXZY)*E(223)+VP4
C      3133 
      VP4=(OPZX*V0033+OPZO*C1033+OPOX*C3033+OQZZ*GZXOO
     1+OQZOZ*GZXZO+GZXZZ)*E(224)+VP4
C      3211 
      VP4=(OPZY*V0011+OPZO*C2011+OPOY*C3011+OQXX*GZYOO
     1+OQXOX*GZYXO+GZYXX)*E(230)+VP4
C      3212 
      VP4=(OPZY*V0012+OPZO*C2012+OPOY*C3012+OQXY*GZYOO
     1+OQXO*GZYYO+OQOY*GZYXO+GZYXY)*E(231)+VP4
C      3213 
      VP4=(OPZY*V0013+OPZO*C2013+OPOY*C3013+OQXZ*GZYOO
     1+OQXO*GZYZO+OQOZ*GZYXO+GZYXZ)*E(232)+VP4
C      3221 
      VP4=(OPZY*V0021+OPZO*C2021+OPOY*C3021+OQYX*GZYOO
     1+OQYO*GZYXO+OQOX*GZYYO+GZYYX)*E(234)+VP4
C      3222 
      VP4=(OPZY*V0022+OPZO*C2022+OPOY*C3022+OQYY*GZYOO
     1+OQYOY*GZYYO+GZYYY)*E(235)+VP4
C      3223 
      VP4=(OPZY*V0023+OPZO*C2023+OPOY*C3023+OQYZ*GZYOO
     1+OQYO*GZYZO+OQOZ*GZYYO+GZYYZ)*E(236)+VP4
C      3231 
      VP4=(OPZY*V0031+OPZO*C2031+OPOY*C3031+OQZX*GZYOO
     1+OQZO*GZYXO+OQOX*GZYZO+GZYZX)*E(238)+VP4
C      3232 
      VP4=(OPZY*V0032+OPZO*C2032+OPOY*C3032+OQZY*GZYOO
     1+OQZO*GZYYO+OQOY*GZYZO+GZYZY)*E(239)+VP4
C      3233 
      VP4=(OPZY*V0033+OPZO*C2033+OPOY*C3033+OQZZ*GZYOO
     1+OQZOZ*GZYZO+GZYZZ)*E(240)+VP4
C      3311 
      VP4=(OPZZ*V0011+OPZOZ*C3011+OQXX*GZZOO
     1+OQXOX*GZZXO+GZZXX)*E(246)+VP4
C      3312 
      VP4=(OPZZ*V0012+OPZOZ*C3012+OQXY*GZZOO
     1+OQXO*GZZYO+OQOY*GZZXO+GZZXY)*E(247)+VP4
C      3313 
      VP4=(OPZZ*V0013+OPZOZ*C3013+OQXZ*GZZOO
     1+OQXO*GZZZO+OQOZ*GZZXO+GZZXZ)*E(248)+VP4
C      3321 
      VP4=(OPZZ*V0021+OPZOZ*C3021+OQYX*GZZOO
     1+OQYO*GZZXO+OQOX*GZZYO+GZZYX)*E(250)+VP4
C      3322 
      VP4=(OPZZ*V0022+OPZOZ*C3022+OQYY*GZZOO
     1+OQYOY*GZZYO+GZZYY)*E(251)+VP4
C      3323 
      VP4=(OPZZ*V0023+OPZOZ*C3023+OQYZ*GZZOO
     1+OQYO*GZZZO+OQOZ*GZZYO+GZZYZ)*E(252)+VP4
C      3331 
      VP4=(OPZZ*V0031+OPZOZ*C3031+OQZX*GZZOO
     1+OQZO*GZZXO+OQOX*GZZZO+GZZZX)*E(254)+VP4
C      3332 
      VP4=(OPZZ*V0032+OPZOZ*C3032+OQZY*GZZOO
     1+OQZO*GZZYO+OQOY*GZZZO+GZZZY)*E(255)+VP4
C      3333 
      VP4=(OPZZ*V0033+OPZOZ*C3033+OQZZ*GZZOO
     1+OQZOZ*GZZZO+GZZZZ)*E(256)+VP4
      VPPPP=VP4*CPPPP
      VE00=VE00+VPPPP
      VE14=VE14+(V1110*E( 86)+V1120*E( 90)+V1130*E( 94)
     *          +V1210*E(102)+V1220*E(106)+V1230*E(110)
     *          +V1310*E(118)+V1320*E(122)+V1330*E(126)
     *          +V2110*E(150)+V2120*E(154)+V2130*E(158)
     *          +V2210*E(166)+V2220*E(170)+V2230*E(174)
     *          +V2310*E(182)+V2320*E(186)+V2330*E(190)
     *          +V3110*E(214)+V3120*E(218)+V3130*E(222)
     *          +V3210*E(230)+V3220*E(234)+V3230*E(238)
     *          +V3310*E(246)+V3320*E(250)+V3330*E(254))*CPPPP
      VE24=VE24+(V1110*E( 87)+V1120*E( 91)+V1130*E( 95)
     *          +V1210*E(103)+V1220*E(107)+V1230*E(111)
     *          +V1310*E(119)+V1320*E(123)+V1330*E(127)
     *          +V2110*E(151)+V2120*E(155)+V2130*E(159)
     *          +V2210*E(167)+V2220*E(171)+V2230*E(175)
     *          +V2310*E(183)+V2320*E(187)+V2330*E(191)
     *          +V3110*E(215)+V3120*E(219)+V3130*E(223)
     *          +V3210*E(231)+V3220*E(235)+V3230*E(239)
     *          +V3310*E(247)+V3320*E(251)+V3330*E(255))*CPPPP
      VE34=VE34+(V1110*E( 88)+V1120*E( 92)+V1130*E( 96)
     *          +V1210*E(104)+V1220*E(108)+V1230*E(112)
     *          +V1310*E(120)+V1320*E(124)+V1330*E(128)
     *          +V2110*E(152)+V2120*E(156)+V2130*E(160)
     *          +V2210*E(168)+V2220*E(172)+V2230*E(176)
     *          +V2310*E(184)+V2320*E(188)+V2330*E(192)
     *          +V3110*E(216)+V3120*E(220)+V3130*E(224)
     *          +V3210*E(232)+V3220*E(236)+V3230*E(240)
     *          +V3310*E(248)+V3320*E(252)+V3330*E(256))*CPPPP
      VE13=VE13+(V1101*E( 86)+V1102*E( 87)+V1103*E( 88)
     *          +V1201*E(102)+V1202*E(103)+V1203*E(104)
     *          +V1301*E(118)+V1302*E(119)+V1303*E(120)
     *          +V2101*E(150)+V2102*E(151)+V2103*E(152)
     *          +V2201*E(166)+V2202*E(167)+V2203*E(168)
     *          +V2301*E(182)+V2302*E(183)+V2303*E(184)
     *          +V3101*E(214)+V3102*E(215)+V3103*E(216)
     *          +V3201*E(230)+V3202*E(231)+V3203*E(232)
     *          +V3301*E(246)+V3302*E(247)+V3303*E(248))*CPPPP
      VE23=VE23+(V1101*E( 90)+V1102*E( 91)+V1103*E( 92)
     *          +V1201*E(106)+V1202*E(107)+V1203*E(108)
     *          +V1301*E(122)+V1302*E(123)+V1303*E(124)
     *          +V2101*E(154)+V2102*E(155)+V2103*E(156)
     *          +V2201*E(170)+V2202*E(171)+V2203*E(172)
     *          +V2301*E(186)+V2302*E(187)+V2303*E(188)
     *          +V3101*E(218)+V3102*E(219)+V3103*E(220)
     *          +V3201*E(234)+V3202*E(235)+V3203*E(236)
     *          +V3301*E(250)+V3302*E(251)+V3303*E(252))*CPPPP
      VE33=VE33+(V1101*E( 94)+V1102*E( 95)+V1103*E( 96)
     *          +V1201*E(110)+V1202*E(111)+V1203*E(112)
     *          +V1301*E(126)+V1302*E(127)+V1303*E(128)
     *          +V2101*E(158)+V2102*E(159)+V2103*E(160)
     *          +V2201*E(174)+V2202*E(175)+V2203*E(176)
     *          +V2301*E(190)+V2302*E(191)+V2303*E(192)
     *          +V3101*E(222)+V3102*E(223)+V3103*E(224)
     *          +V3201*E(238)+V3202*E(239)+V3203*E(240)
     *          +V3301*E(254)+V3302*E(255)+V3303*E(256))*CPPPP
      VE12=VE12+(V1011*E( 86)+V1012*E( 87)+V1013*E( 88)
     *          +V1021*E( 90)+V1022*E( 91)+V1023*E( 92)
     *          +V1031*E( 94)+V1032*E( 95)+V1033*E( 96)
     *          +V2011*E(150)+V2012*E(151)+V2013*E(152)
     *          +V2021*E(154)+V2022*E(155)+V2023*E(156)
     *          +V2031*E(158)+V2032*E(159)+V2033*E(160)
     *          +V3011*E(214)+V3012*E(215)+V3013*E(216)
     *          +V3021*E(218)+V3022*E(219)+V3023*E(220)
     *          +V3031*E(222)+V3032*E(223)+V3033*E(224))*CPPPP
      VE22=VE22+(V1011*E(102)+V1012*E(103)+V1013*E(104)
     *          +V1021*E(106)+V1022*E(107)+V1023*E(108)
     *          +V1031*E(110)+V1032*E(111)+V1033*E(112)
     *          +V2011*E(166)+V2012*E(167)+V2013*E(168)
     *          +V2021*E(170)+V2022*E(171)+V2023*E(172)
     *          +V2031*E(174)+V2032*E(175)+V2033*E(176)
     *          +V3011*E(230)+V3012*E(231)+V3013*E(232)
     *          +V3021*E(234)+V3022*E(235)+V3023*E(236)
     *          +V3031*E(238)+V3032*E(239)+V3033*E(240))*CPPPP
      VE32=VE32+(V1011*E(118)+V1012*E(119)+V1013*E(120)
     *          +V1021*E(122)+V1022*E(123)+V1023*E(124)
     *          +V1031*E(126)+V1032*E(127)+V1033*E(128)
     *          +V2011*E(182)+V2012*E(183)+V2013*E(184)
     *          +V2021*E(186)+V2022*E(187)+V2023*E(188)
     *          +V2031*E(190)+V2032*E(191)+V2033*E(192)
     *          +V3011*E(246)+V3012*E(247)+V3013*E(248)
     *          +V3021*E(250)+V3022*E(251)+V3023*E(252)
     *          +V3031*E(254)+V3032*E(255)+V3033*E(256))*CPPPP
      VE11=VE11+(V0111*E( 86)+V0112*E( 87)+V0113*E( 88)
     *          +V0121*E( 90)+V0122*E( 91)+V0123*E( 92)
     *          +V0131*E( 94)+V0132*E( 95)+V0133*E( 96)
     *          +V0211*E(102)+V0212*E(103)+V0213*E(104)
     *          +V0221*E(106)+V0222*E(107)+V0223*E(108)
     *          +V0231*E(110)+V0232*E(111)+V0233*E(112)
     *          +V0311*E(118)+V0312*E(119)+V0313*E(120)
     *          +V0321*E(122)+V0322*E(123)+V0323*E(124)
     *          +V0331*E(126)+V0332*E(127)+V0333*E(128))*CPPPP
      VE21=VE21+(V0111*E(150)+V0112*E(151)+V0113*E(152)
     *          +V0121*E(154)+V0122*E(155)+V0123*E(156)
     *          +V0131*E(158)+V0132*E(159)+V0133*E(160)
     *          +V0211*E(166)+V0212*E(167)+V0213*E(168)
     *          +V0221*E(170)+V0222*E(171)+V0223*E(172)
     *          +V0231*E(174)+V0232*E(175)+V0233*E(176)
     *          +V0311*E(182)+V0312*E(183)+V0313*E(184)
     *          +V0321*E(186)+V0322*E(187)+V0323*E(188)
     *          +V0331*E(190)+V0332*E(191)+V0333*E(192))*CPPPP
      VE31=VE31+(V0111*E(214)+V0112*E(215)+V0113*E(216)
     *          +V0121*E(218)+V0122*E(219)+V0123*E(220)
     *          +V0131*E(222)+V0132*E(223)+V0133*E(224)
     *          +V0211*E(230)+V0212*E(231)+V0213*E(232)
     *          +V0221*E(234)+V0222*E(235)+V0223*E(236)
     *          +V0231*E(238)+V0232*E(239)+V0233*E(240)
     *          +V0311*E(246)+V0312*E(247)+V0313*E(248)
     *          +V0321*E(250)+V0322*E(251)+V0323*E(252)
     *          +V0331*E(254)+V0332*E(255)+V0333*E(256))*CPPPP
      RETURN
      END
