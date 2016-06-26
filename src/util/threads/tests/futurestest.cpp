/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "futurestest.h"
#include <QEventLoop>
#include <QtTest>
#include <futures.h>
#include "common.h"

QTEST_MAIN (LeechCraft::Util::FuturesTest)

namespace LeechCraft
{
namespace Util
{
	void FuturesTest::testSequencer ()
	{
		QEventLoop loop;
		int res = 0;
		Sequence (nullptr, MkWaiter () (25))
			.Then (MkWaiter ())
			.Then (MkWaiter ())
			.Then (MkWaiter ())
			.Then ([&loop, &res] (int cnt)
					{
						res = cnt;
						loop.quit ();
					});

		loop.exec ();

		QCOMPARE (res, 400);
	}

	void FuturesTest::testHeterogeneousTypes ()
	{
		struct Bar {};
		struct Baz {};

		QEventLoop loop;
		bool executed = false;
		Sequence (nullptr, MkWaiter () (50)) >>
				[] (int) { return MakeReadyFuture<Bar> ({}); } >>
				[] (Bar) { return MakeReadyFuture<Baz> ({}); } >>
				[&executed, &loop] (Baz)
				{
					executed = true;
					loop.quit ();
				};

		loop.exec ();

		QCOMPARE (executed, true);
	}

	void FuturesTest::testDestruction ()
	{
		QEventLoop loop;
		bool executed = false;

		{
			QObject obj;
			Sequence (&obj, MakeReadyFuture (0)) >>
					[&executed, &loop] (int)
					{
						executed = true;
						loop.quit ();
					};
		}

		QTimer::singleShot (100, &loop, SLOT (quit ()));

		loop.exec ();

		QCOMPARE (executed, false);
	}

	void FuturesTest::testDestructionHandler ()
	{
		const auto finished = 1;
		const auto destructed = 2;

		QEventLoop loop;
		bool executed = false;
		int value = 0;

		QFuture<int> future;
		{
			QObject obj;
			future = Sequence (&obj, MkWaiter () (100))
					.DestructionValue ([destructed] { return destructed; }) >>
					[=] (int) { return MakeReadyFuture (finished); };
		}
		Sequence (nullptr, future) >>
				[&executed, &value, &loop] (int val)
				{
					value = val;
					executed = true;
					loop.quit ();
				};

		QTimer::singleShot (10, &loop, SLOT (quit ()));

		loop.exec ();

		QCOMPARE (executed, true);
		QCOMPARE (value, destructed);
	}

	void FuturesTest::testNoDestrHandler ()
	{
		struct Bar {};
		struct Baz {};

		QEventLoop loop;
		bool executed = false;
		Sequence (nullptr, MkWaiter () (50))
				.DestructionValue ([&executed] { executed = true; }) >>
				[] (int) { return MakeReadyFuture<Bar> ({}); } >>
				[] (Bar) { return MakeReadyFuture<Baz> ({}); } >>
				[&loop] (Baz) { loop.quit (); };

		loop.exec ();

		QCOMPARE (executed, false);
	}

	void FuturesTest::testNoDestrHandlerSetBuildable ()
	{
		const auto finished = 1;

		QEventLoop loop;
		bool executed = false;
		int value = 0;

		QFuture<int> future = Sequence (nullptr, MkWaiter () (10)) >>
				[=] (int) { return MakeReadyFuture (finished); };
		Sequence (nullptr, future) >>
				[&executed, &value, &loop] (int val)
				{
					value = val;
					executed = true;
					loop.quit ();
				};

		loop.exec ();

		QCOMPARE (executed, true);
		QCOMPARE (value, finished);
	}

	void FuturesTest::testMulti ()
	{
		QEventLoop loop;

		QFutureInterface<int> iface;

		int count = 0;
		int sum = 0;
		Sequence (nullptr, iface.future ())
				.MultipleResults ([&] (int sub)
						{
							sum += sub;
							++count;
						},
						[&] { loop.quit (); });

		iface.reportStarted ();
		iface.setProgressRange (0, 2);
		iface.reportResult (1, 0);
		iface.reportResult (2, 1);
		iface.reportResult (3, 2);
		iface.reportFinished ();

		loop.exec ();

		QCOMPARE (count, 3);
		QCOMPARE (sum, 6);
	}
}
}