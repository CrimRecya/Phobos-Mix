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

	// Type-erased base for observable vectors.
	// Lets containers (PageView / IconStrip / ListGrid) bind to any ObservableVector<T>
	// without knowing T at compile time.
	class ObservableVectorBase
	{
	public:
		using ChangedCallback = std::function<void()>;

		virtual ~ObservableVectorBase() = default;

		ObservableVectorBase(const ObservableVectorBase&) = delete;
		ObservableVectorBase& operator=(const ObservableVectorBase&) = delete;

		virtual size_t GetCount() const = 0;
		virtual const void* GetItem(size_t index) const = 0;
		virtual void SetOnChanged(ChangedCallback callback) = 0;

	protected:
		ObservableVectorBase() = default;
	};

	// A minimal observable vector.
	// Useful for lists of items that will be rendered by PageView / IconStrip / ListGrid.
	template <typename T>
	class ObservableVector final : public ObservableVectorBase
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
			this->NotifyChanged();
		}

		void Add(const T& item)
		{
			this->Items_.push_back(item);
			++this->Revision_;
			this->NotifyChanged();
		}

		void Add(T&& item)
		{
			this->Items_.emplace_back(std::move(item));
			++this->Revision_;
			this->NotifyChanged();
		}

		void RemoveAt(size_t index)
		{
			if (index < this->Items_.size())
			{
				this->Items_.erase(this->Items_.begin() + index);
				++this->Revision_;
				this->NotifyChanged();
			}
		}

		void Clear()
		{
			if (!this->Items_.empty())
			{
				this->Items_.clear();
				++this->Revision_;
				this->NotifyChanged();
			}
		}

		size_t GetRevision() const
		{
			return this->Revision_;
		}

		// ObservableVectorBase
		size_t GetCount() const override
		{
			return this->Items_.size();
		}

		const void* GetItem(size_t index) const override
		{
			return &this->Items_[index];
		}

		void SetOnChanged(ChangedCallback callback) override
		{
			this->OnChanged_ = std::move(callback);
		}

	private:
		void NotifyChanged()
		{
			if (this->OnChanged_)
				this->OnChanged_();
		}

		std::vector<T> Items_ {};
		size_t Revision_ { 0 };
		ChangedCallback OnChanged_ { };
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
			, LastRevision_ { static_cast<size_t>(-1) }
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
		using CanExecuteChangedFn = std::function<void()>;

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
			this->NotifyCanExecuteChanged();
		}

		size_t AddCanExecuteChanged(CanExecuteChangedFn callback)
		{
			if (!callback)
				return static_cast<size_t>(-1);

			this->CanExecuteChangedCallbacks_.push_back(std::move(callback));
			return this->CanExecuteChangedCallbacks_.size() - 1;
		}

		void RemoveCanExecuteChanged(size_t token)
		{
			if (token < this->CanExecuteChangedCallbacks_.size())
				this->CanExecuteChangedCallbacks_[token] = nullptr;
		}

		void NotifyCanExecuteChanged() const
		{
			for (const auto& callback : this->CanExecuteChangedCallbacks_)
			{
				if (callback)
					callback();
			}
		}

	private:
		ExecuteFn Execute_ {};
		CanExecuteFn CanExecute_ {};
		std::vector<CanExecuteChangedFn> CanExecuteChangedCallbacks_ { };
	};
}