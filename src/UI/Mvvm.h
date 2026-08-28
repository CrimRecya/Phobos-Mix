#pragma once

#include <functional>
#include <utility>
#include <vector>
#include <cstddef>

namespace UIExt
{
	// A minimal observable value.
	// Changes bump a revision instead of invoking listeners.
	// Bindings compare revisions and apply the new value on the next UIRoot flush.
	template <typename T>
	class Observable final
	{
	public:
		Observable() = default;
		Observable(T value) : Value_ { std::move(value) } { }

		const T& Get() const
		{
			return this->Value_;
		}

		void Set(const T& value)
		{
			this->Value_ = value;
			++this->Revision_;
		}

		void Set(T&& value)
		{
			this->Value_ = std::move(value);
			++this->Revision_;
		}

		size_t GetRevision() const
		{
			return this->Revision_;
		}

	private:
		T Value_ {};
		size_t Revision_ { 0 };
	};

	// A minimal observable vector.
	// Useful for lists of items that will be rendered by PageView / IconStrip / ListGrid.
	template <typename T>
	class ObservableVector final
	{
	public:
		ObservableVector() = default;
		explicit ObservableVector(std::vector<T> items) : Items_ { std::move(items) } { }

		const std::vector<T>& GetItems() const
		{
			return this->Items_;
		}

		void SetItems(std::vector<T> items)
		{
			this->Items_ = std::move(items);
			++this->Revision_;
		}

		void Add(const T& item)
		{
			this->Items_.push_back(item);
			++this->Revision_;
		}

		void Add(T&& item)
		{
			this->Items_.emplace_back(std::move(item));
			++this->Revision_;
		}

		void RemoveAt(size_t index)
		{
			if (index < this->Items_.size())
			{
				this->Items_.erase(this->Items_.begin() + index);
				++this->Revision_;
			}
		}

		void Clear()
		{
			if (!this->Items_.empty())
			{
				this->Items_.clear();
				++this->Revision_;
			}
		}

		size_t GetRevision() const
		{
			return this->Revision_;
		}

	private:
		std::vector<T> Items_ {};
		size_t Revision_ { 0 };
	};

	// BindingBase is the polymorphic root for value bindings.
	// UIRoot flushes all bindings once per frame.
	class BindingBase
	{
	public:
		virtual ~BindingBase() = default;

		BindingBase(const BindingBase&) = delete;
		BindingBase& operator=(const BindingBase&) = delete;

		virtual void ApplyIfNeeded() = 0;

	protected:
		BindingBase() = default;
	};

	// A binding from an Observable<T> to a setter on a control.
	// The observable must outlive the binding; this is the usual ViewModel-lifetime case.
	template <typename T>
	class ValueBinding final : public BindingBase
	{
	public:
		using Setter = std::function<void(const T&)>;

		ValueBinding(const Observable<T>* observable, Setter setter)
			: Observable_ { observable }
			, Setter_ { std::move(setter) }
			, LastRevision_ { observable ? observable->GetRevision() : 0 }
		{ }

		void ApplyIfNeeded() override
		{
			if (!Observable_)
				return;

			const auto revision = Observable_->GetRevision();

			if (revision != this->LastRevision_)
			{
				this->LastRevision_ = revision;
				if (this->Setter_)
					this->Setter_(Observable_->Get());
			}
		}

	private:
		const Observable<T>* Observable_ { nullptr };
		Setter Setter_ {};
		size_t LastRevision_ { 0 };
	};

	// A simple command object. CanExecute is optional.
	class Command final
	{
	public:
		using ExecuteFn = std::function<void()>;
		using CanExecuteFn = std::function<bool()>;

		Command() = default;
		Command(ExecuteFn execute) : Execute_ { std::move(execute) } { }
		Command(ExecuteFn execute, CanExecuteFn canExecute)
			: Execute_ { std::move(execute) }
			, CanExecute_ { std::move(canExecute) }
		{ }

		void Execute() const
		{
			if (this->Execute_)
				this->Execute_();
		}

		bool CanExecute() const
		{
			return !this->CanExecute_ || this->CanExecute_();
		}

		void SetExecute(ExecuteFn execute)
		{
			this->Execute_ = std::move(execute);
		}

		void SetCanExecute(CanExecuteFn canExecute)
		{
			this->CanExecute_ = std::move(canExecute);
		}

	private:
		ExecuteFn Execute_ {};
		CanExecuteFn CanExecute_ {};
	};
}