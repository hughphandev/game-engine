#include "tests/test.h"
#include "main/math.cpp"

int main(int argc, char const* argv[])
{

    {
        mat4 m;
        m.r0 = { 2, 4, 6, 0 };
        m.r1 = { 3, 1, 0, 5 };
        m.r2 = { 4, 1, 1, 9 };
        m.r3 = { 5, 2, 0, 1 };

        mat4 e;
        e.r0 = { 1, 0, 0, 0 };
        e.r1 = { 0, 1, 0, 0 };
        e.r2 = { 0, 0, 1, 0 };
        e.r3 = { 0, 0, 0, 1 };

        TEST_EQUAL(m * e, m);
        TEST_EQUAL(e * m, m);
        TEST_EQUAL(m * e, e * m);
    }

    {
        mat4 a;
        a.r0 = { 5, 7, 9, 10 };
        a.r1 = { 2, 3, 3, 8 };
        a.r2 = { 8, 10, 2, 3 };
        a.r3 = { 3, 3, 4, 8 };

        mat4 b;
        b.r0 = { 3, 10, 12, 18 };
        b.r1 = { 12, 1, 4, 9 };
        b.r2 = { 9, 10, 12, 2 };
        b.r3 = { 3, 12, 4, 10 };

        mat4 c;
        c.r0 = { 210, 267, 236, 271 };
        c.r1 = { 93, 149, 104, 149 };
        c.r2 = { 171, 146, 172, 268 };
        c.r3 = { 105, 169, 128, 169 };
        TEST_EQUAL(a * b, c);
    }

    {
        mat4 e;
        e.r0 = { 1, 0, 0, 0 };
        e.r1 = { 0, 1, 0, 0 };
        e.r2 = { 0, 0, 1, 0 };
        e.r3 = { 0, 0, 0, 1 };

        v4 a = { 1, 2, 3, 4 };

        TEST_EQUAL(a * e, a);
        TEST_EQUAL(e * a, a);
    }

    {
        mat4 M;
        M.r0 = { 1, 0, 0, 0 };
        M.r1 = { 1, 2, 0, 0 };
        M.r2 = { 0, 0, 1, 0 };
        M.r3 = { 0, 0, 0, 1 };

        v4 a = { 1, 2, 3, 4 };

        v4 axM = { 3, 4, 3, 4 };
        v4 Mxa = { 1, 5, 3, 4 };

        TEST_EQUAL(a * M, axM);
        TEST_EQUAL(M * a, Mxa);

    }

    {
        v3 a = V3(1, 0, 0);
        v3 b = V3(0, 1, 0);

        v3 aCrossb = V3(0, 0, 1);
        TEST_EQUAL(Cross(a, b), aCrossb);
        TEST_EQUAL(Cross(b, a), -aCrossb);
    }

    {
        mat4 M;
        M.r0 = { 1, 0, 0, 0 };
        M.r1 = { 1, 2, 0, 0 };
        M.r2 = { 0, 0, 1, 0 };
        M.r3 = { 0, 0, 0, 1 };

        mat4 M1;
        M1.r0 = { 1, 2, 3, 4 };
        M1.r1 = { 1, 2, 3, 4 };
        M1.r2 = { 1, 2, 3, 4 };
        M1.r3 = { 1, 2, 3, 4 };

        mat4 M2;
        M2.r0 = { 1, 3, 4, 5 };
        M2.r1 = { 2, 1, 2, 3 };
        M2.r2 = { 3, 4, 1, 2 };
        M2.r3 = { 4, 4, 5, 6 };

        TEST_EQUAL(Det(M), 2);
        TEST_EQUAL(Det(M1), 0);
        TEST_EQUAL(Det(M2), -28);
    }

    {
        mat4 M2;
        M2.r0 = { 1, 3, 4, 5 };
        M2.r1 = { 2, 1, 2, 3 };
        M2.r2 = { 3, 4, 1, 2 };
        M2.r3 = { 4, 4, 5, 6 };

        mat4 M1;
        M1.r0 = { 1, 2, 3, 4 };
        M1.r1 = { 1, 2, 3, 4 };
        M1.r2 = { 1, 2, 3, 4 };
        M1.r3 = { 1, 2, 3, 4 };

        mat4 M;
        M.r0 = { 1, 0, 0, 0 };
        M.r1 = { 0, 2, 0, 0 };
        M.r2 = { 0, 0, 1, 0 };
        M.r3 = { 0, 0, 0, 1 };

        mat4 invM;
        invM.r0 = { 1, 0, 0, 0 };
        invM.r1 = { 0, 0.5f, 0, 0 };
        invM.r2 = { 0, 0, 1, 0 };
        invM.r3 = { 0, 0, 0, 1 };

        TEST_EQUAL(Inverse(M1).isExisted, false);
        TEST_EQUAL(Inverse(M2).isExisted, true);
        TEST_EQUAL(Inverse(M).isExisted, true);
        TEST_EQUAL(Inverse(M).det, 2.0f);
        TEST_EQUAL(Inverse(M).inv, invM);
    }

    return 0;
}
