#pragma once
//==============================================================================================
//  rect.h                      rect, image map, etc [***..]
//  CRUXELL.RND.software		jin lee
//  2018.01.24					2018.03.07
//------------------------------+---------------------------------------------------------------

namespace                       crux
{
    //------------------------------+---------------------------------------------------------------
    //  rectangle

    ttt class                       XY2
    {
        using                       A = AB<T>;
        using                       B = XY<T>;
        using                       C = XY2<T>;

    protected:
        std::array<B, 2>             v;

    public:
        ///-------------------------+---------------------------------------------------------------

        auto&                       p0() { return  v[0]; }
        auto&                       p1() { return  v[1]; }
        auto                        p0() co { return  v[0]; }
        auto                        p1() co { return  v[1]; }

        CRE                     cex XY2() {}
        CRE                     cex XY2(C&& r) : v{ r.p0(), r.p1() } {}
        CRE                     cex XY2(co C& r) : v{ r.p0(), r.p1() } {}
        CRE tt< tn U, tn V >    cex XY2(XY<U>&& a, XY<V>&& b) : v{ B(a), B(b) } {}
        CRE tt< tn U, tn V >    cex XY2(co XY<U>& a, co XY<V>& b) : v{ B(a), B(b) } {}
        CRE tt< tn U, tn V >    cex XY2(AB<U>&& a, AB<V>&& b) : v{ A(a), A(b) } {}
        CRE tt< tn U, tn V >    cex XY2(co AB<U>& a, co AB<V>& b) : v{ A(a), A(b) } {}
        CRE tt< tn U, tn V, tn W, tn X >
            cex XY2(co U& a, co V&b, co W& c, co X& d)
        {
            v[0] = { (T)a, (T)b }, v[1] = { (T)c, (T)d };
        }

        auto&                       left() { return  p0().x(); }
        auto&                       top() { return  p0().y(); }
        auto&                       right() { return  p1().x(); }
        auto&                       bottom() { return  p1().y(); }
        auto                        left()   co { return  p0().x(); }
        auto                        top()    co { return  p0().y(); }
        auto                        right()  co { return  p1().x(); }
        auto                        bottom() co { return  p1().y(); }

        ///-------------------------+---------------------------------------------------------------

        auto&                       operator=(co C& r)
        {
            return  (this->v = r.v), *this;
        }

        auto&                       operator=(C&& r)
        {
            return  (this->v = r.v), *this;
        }

        tt< tn U >auto&             operator=(co XY2<U>& r)
        {
            return  (this->p0() = r.p0(), this->p1() = r.p1()), *this;
        }

        tt< tn U >auto&             operator=(XY2<U>&& r)
        {
            return  (this->p0() = r.p0(), this->p1() = r.p1()), *this;
        }
        ///-------------------------+---------------------------------------------------------------

        auto&                       operator+=(co C& r)
        {
            return  (this->p0() += r.p0(), this->p1() += r.p1()), *this;
        }

        auto&                       operator-=(co C& r)
        {
            return  (this->p0() -= r.p0(), this->p1() -= r.p1()), *this;
        }

        auto&                       operator*=(co C& r)
        {
            return  (this->p0() *= r.p0(), this->p1() *= r.p1()), *this;
        }

        auto&                       operator/=(co C& r)
        {
            return  (this->p0() /= r.p0(), this->p1() /= r.p1()), *this;
        }
        ///-------------------------+---------------------------------------------------------------

        auto&                       operator+=(co T& r)
        {
            return  (this->p0() += r, this->p1() += r), *this;
        }

        auto&                       operator-=(co T& r)
        {
            return  (this->p0() -= r, this->p1() -= r), *this;
        }

        auto&                       operator*=(co T& r)
        {
            return  (this->p0() *= r, this->p1() *= r), *this;
        }

        auto&                       operator/=(co T& r)
        {
            return  (this->p0() /= r, this->p1() /= r), *this;
        }

        ///-------------------------+---------------------------------------------------------------

        auto                        operator+(co C& r) co
        {
            return  C(*this) += r;
        }

        auto                        operator-(co C& r) co
        {
            return  C(*this) -= r;
        }

        auto                        operator*(co C& r) co
        {
            return  C(*this) *= r;
        }

        auto                        operator/(co C& r) co
        {
            return  C(*this) /= r;
        }

        ///-------------------------+---------------------------------------------------------------

        auto                        operator+(co T& r) co
        {
            return  C{ *this } += r;
        }

        auto                        operator-(co T& r) co
        {
            return  C{ *this } -= r;
        }

        auto                        operator*(co T& r) co
        {
            return  C{ *this } *= r;
        }

        auto                        operator/(co T& r) co
        {
            return  C{ *this } /= r;
        }
        ///-------------------------+---------------------------------------------------------------

        auto                        operator-() co
        {
            return  C{ (B)-this->p0(), (B)-this->p1() };
        }

        auto                        dx() co
        {
            return  right() - left();
        }

        auto                        dy() co
        {
            return  bottom() - top();
        }

        auto                        d() co
        {
            return  XY<T>{ dx(), dy() };
        }

        auto                        length_sqr() co
        {
            return  dx() * dx() + dy() * dy();
        }

        auto                        length() co
        {
            return  std::sqrt(length_sqr());
        }

        auto                        has(co XY<T>& pos) co                                         NOTICE("ends included")
        {
            return  pos.x() >= left() && pos.x() <= right() &&
                pos.y() >= top() && pos.y() <= bottom();
        }

        auto                        size() co
        {
            return  dx() * dy();
        }

        auto&                       size_to(co XY<T>& size)
        {
            p1() = p0() + size;
            return  *this;
        }

        auto&                       move_by(co XY<T>& delta)
        {
            p0() += delta;          p1() += delta;
            return  *this;
        }

        auto&                       grow_by(co XY<T>& delta)
        {
            p0() -= delta;          p1() += delta;
            return  *this;
        }

        auto&                       sort()
        {
            if (left() > right())
                std::swap(left(), right());
            if (top() > bottom())
                std::swap(top(), bottom());
            return  *this;
        }
        ///-------------------------+---------------------------------------------------------------

        auto                        str() co
        {
            return  STR("[ ") + p0().str() + ", " + p1().str() + " ]";
        }
    };
    //------------------------------+---------------------------------------------------------------
   
    //------------------------------+---------------------------------------------------------------
    //  map for image

#define forx_xy2( xy2 )         fo( x, xy2.left(); xy2.right() )
#define fory_xy2( xy2 )         fo( y, xy2.top(); xy2.bottom() )

#define map_parallel_loop( ... ) \
    if( multi ) parallel_for( left(), right(), [ __VA_ARGS__, &f ]( i32 x ) \
        { f( __VA_ARGS__, x ); } ); \
    else forx_xy2( roi ) f( __VA_ARGS__, x );

///-----------------------------+---------------------------------------------------------------

    ttt class                       MAP : public LIST<T>
    {
        using   sample_t = T;
        XY<i32>                     siz;

        ///-------------------------+---------------------------------------------------------------

        struct                      LINES
        {
            MAP<T>&                 my;
            CRE                     LINES(MAP<T>& my) : my(my) {}
            auto                    operator[](int index) co
            {
                assert(index >= 0 && index < my.h());
                return  my.data() + index * my.w();
            }
            auto                    operator()(int index)
            {
                return  range_sub(my, my.w() * index, my.w());
            }
        };
        ///-------------------------+---------------------------------------------------------------

        struct                      BORDER
        {
            MAP<T>&                 my;
            CRE                     BORDER(MAP<T>& my) : my(my) {}
            auto                    operator=(MAP<T>& r)
            {
                fo(y, 0; my.top())
                    crux::copy(my.lines[y], r.lines(y));
                fo(y, my.top(); my.bottom())
                {
                    auto    s = r.lines[y];
                    auto    d = my.lines[y];
                    fo(x, 0; my.left()) d[x] = s[x];
                    fo(x, my.right(); my.w()) d[x] = s[x];
                }
                fo(y, my.bottom(); my.h())
                    crux::copy(my.lines[y], r.lines(y));
            }

            auto                    operator=(co T& value)
            {
                fo(y, 0; my.top())
                    for (auto& dst : my.lines(y)) dst = value;
                fo(y, my.top(); my.bottom())
                {
                    auto    d = my.lines[y];
                    fo(x, 0; my.left()) d[x] = value;
                    fo(x, my.right(); my.w()) d[x] = value;
                }
                fo(y, my.bottom(); my.h())
                    for (auto& dst : my.lines(y)) dst = value;
            }
        };
        ///-------------------------+---------------------------------------------------------------

    public:
        LINES                       lines;
        BORDER                      border;

        XY<i32>                     res;
        XY2<i32>                    roi;

        auto                        left() { return  roi.left(); }
        auto                        top() { return  roi.top(); }
        auto                        right() { return  roi.right(); }
        auto                        bottom() { return  roi.bottom(); }

        CRE                         MAP(co XY<i32> size, co XY<i32> res = { 0, 0 })
            : siz(size), res(res)
            , lines(*this), border(*this)
            , LIST<T>(0)
        {
            resize(siz.x() * siz.y());
            reset_roi();
        }

        CRE                         MAP(co MAP<T>& r)
            : siz(r.siz), res(r.res)
            , lines(*this), border(*this)
            , LIST<T>(0)
        {
            resize(r.size());
            crux::copy(data(), r.data(), size());
        }

        ///-------------------------+---------------------------------------------------------------

        auto&                       operator=(co MAP<T>& r)
        {
            siz = r.d();
            reset_roi();
            res = r.res;
            resize(r.size());
            crux::copy(data(), r.data(), size());
            return  *this;
        }

        tt< tn U >          auto&   operator=(co MAP<U>& r)
        {
            siz = r.d();
            reset_roi();
            res = r.res;
            resize(r.size());
            times(r.size()) (*this)[i] = (T)r[i];
            return  *this;
        }

        auto&                       operator=(co T r)
        {
            std::fill(begin(), end(), r);
            return  *this;
        }
        ///-------------------------+---------------------------------------------------------------

        tt< tn U >          auto&   operator+=(MAP<U>& r)
        {
            return  go(r, [](auto a, auto b, i32 x) { a[x] += b[x]; });
        }

        tt< tn U >          auto&   operator-=(MAP<U>& r)
        {
            return  go(r, [](auto a, auto b, i32 x) { a[x] -= b[x]; });
        }

        tt< tn U >          auto&   operator*=(MAP<U>& r)
        {
            return  go(r, [](auto a, auto b, i32 x) { a[x] *= b[x]; });
        }

        tt< tn U >          auto&   operator/=(MAP<U>& r)
        {
            return  go(r, [](auto a, auto b, i32 x) { a[x] /= b[x]; });
        }

        auto&                       abs()
        {
            return  go([](auto a, i32 x) { a[x] = std::fabs(a[x]); });
        }

        auto&                       operator+=(co T& r)
        {
            return  go([r](auto a, i32 x) { a[x] += r; });
        }

        auto&                       operator-=(co T& r)
        {
            return  go([r](auto a, i32 x) { a[x] -= r; });
        }

        auto&                       operator*=(co T& r)
        {
            return  go([r](auto a, i32 x) { a[x] *= r; });
        }

        auto&                       operator/=(co T& r)
        {
            return  go([r](auto a, i32 x) { a[x] /= r; });
        }
        ///-------------------------+---------------------------------------------------------------

        co  auto                    set_roi(co XY2<i32>& roi)
        {
            this->roi = roi;
        }

        co  auto                    make_border(co XY2<i32>& margin)
        {
            roi = XY2<i32>{ margin.p0(), siz - margin.p1() };
        }

        auto                        reset_roi()
        {
            make_border({ 0, 0, 0, 0 });
        }

        auto                        roi_size()
        {
            return  roi.size();
        }

        tt< tn U >          auto    transpose(MAP<U>& dst = *this)
        {
            transpose_xy(dst.data(), data(), w(), h());
            dst.siz = siz.yx();
            dst.reset_roi();
        }
        ///-------------------------+---------------------------------------------------------------

        tt< bool multi = true, tn B, fn F >
            auto&                       GO(MAP<B>& bb, fun(F) f)
        {
            fory_xy2(roi)
            {
                auto    b = bb.lines[y];
                auto    a = lines[y];
                map_parallel_loop(a, b, y);
            }
            reset_roi();
            return  *this;
        }
        ///-------------------------+---------------------------------------------------------------

        tt< bool multi = true, fn F >
            auto&                       go(fun(F) f)
        {
            fory_xy2(roi)
            {
                auto    a = lines[y];
                map_parallel_loop(a);
            }
            reset_roi();
            return  *this;
        }

        tt< bool multi = true, tn B, fn F >
            auto&                       go(MAP<B>& bb, fun(F) f)
        {
            fory_xy2(roi)
            {
                auto    b = bb.lines[y];
                auto    a = lines[y];
                map_parallel_loop(a, b);
            }
            reset_roi();
            return  *this;
        }

        tt< bool multi = true, tn B, fn F, fn F0 >
            auto&                       go(MAP<B>& bb, fun(F0) f0, fun(F) f)
        {
            fory_xy2(roi)
            {
                auto    b = bb.lines[y];
                auto    a = lines[y];
                f0(a, b, roi.p0().x());
                fo(x, left() + 1; right()) f(a, b, x);
            }
            reset_roi();
            return  *this;
        }

        tt< bool multi = true, tn B, tn C, fn F >
            auto&                       go(MAP<B>& bb, MAP<C>& cc, fun(F) f)
        {
            fory_xy2(roi)
            {
                auto    c = cc.lines[y];
                auto    b = bb.lines[y];
                auto    a = lines[y];
                map_parallel_loop(a, b, c);
            }
            reset_roi();
            return  *this;
        }
        ///-------------------------+---------------------------------------------------------------

        auto                        w() co
        {
            return  siz.x();
        }

        auto                        h() co
        {
            return  siz.y();
        }

        auto                        d() co
        {
            return  siz;
        }
        ///-------------------------+---------------------------------------------------------------
		auto                        save(co wchar_t* filename) co
		{
			FILE*   fp = _wfopen(filename, L"wb");
			if (!fp) { return  false; }
			fwrite(data(), size(), sizeof *data(), fp);
			fclose(fp);
			return  true;
		}
		auto                        save(co wchar_t* filename, int w, int h) co
		{
			FILE*   fp = _wfopen(filename, L"wb");
			if (!fp) { return  false; }
			//fwrite(data(), size(), sizeof *data(), fp);
			unsigned short * line = (unsigned short *)data();
			for (int y = 0; y < h; y++)
			{
				fwrite(line, w, sizeof(line[0]), fp);
				line += w;
			}
			fclose(fp);
			return  true;
		}
		auto                        save(co STRING& filename, int w, int h) co
		{
			FILE*   fp = fopen(filename.c_str(), "wb");
			if (!fp) { say(filename + " couldn't save"); return  false; }
			unsigned short * line = (unsigned short *)data();
			for (int y = 0; y < h; y++)
			{
				fwrite(line, w, sizeof(line[0]), fp);
				line += fin.w();
			}
			//fwrite(data(), size(), sizeof *data(), fp);
			fclose(fp);
			return  true;
		}

		
		auto                        save(co STRING& filename) co
        {
            FILE*   fp = fopen(filename.c_str(), "wb");
            if (!fp) { say(filename + " couldn't save"); return  false; }
            fwrite(data(), size(), sizeof *data(), fp);
            fclose(fp);
            return  true;
        }
        ///-------------------------+---------------------------------------------------------------

        auto                        load(co STRING& filename)
        {
            FILE*   fp = fopen(filename.c_str(), "rb");
            if (!fp) { say(filename + " couldn't load"); return  false; }
            fread(data(), size(), sizeof *data(), fp);
            fclose(fp);
            return  true;
        }
    };
    //==============================================================================================
};
