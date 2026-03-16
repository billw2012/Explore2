
#if !defined(__UTILS_QTUTILS_H__)
#define __UTILS_QTUTILS_H__

#if defined(_DEBUG)
#	define ASSERTS_ENABLED
#endif

#if defined(ASSERTS_ENABLED)
#	define ASSERT(x)				assert(x)
#	define REMOVE_WITH_ASSERTS(x)	x
#else
#	define ASSERT(x)
#	define REMOVE_WITH_ASSERTS(x)
#endif

void asserted_connect(QObject* from, const char* signal, QObject* to, const char* slot, Qt::ConnectionType connectionType = Qt::AutoConnection)
{
	REMOVE_WITH_ASSERTS(bool success =) 
		QObject::connect(from, signal, to, slot, connectionType);
	ASSERT(success);
}

struct ScopedSuspendUpdates
{
	ScopedSuspendUpdates(QWidget* widget)
		: _widget(widget)
	{
		_widget->setUpdatesEnabled(false);
	}
	~ScopedSuspendUpdates()
	{
		_widget->setUpdatesEnabled(true);
	}

private:
	QWidget* _widget;
};

#endif // !defined(__UTILS_QTUTILS_H__)