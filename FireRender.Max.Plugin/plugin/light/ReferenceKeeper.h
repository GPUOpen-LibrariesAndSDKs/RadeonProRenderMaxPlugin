#pragma once

#include "frScope.h"

FIRERENDER_NAMESPACE_BEGIN

enum class MaxReferenceType
{
	Strong,
	Weak,
	Indirect
};

class IMaxReference
{
public:
	virtual MaxReferenceType GetType() const = 0;
	virtual ReferenceTarget* GetRef() const = 0;
	virtual void SetRef(ReferenceTarget* ref) = 0;
};

template<typename T>
struct MaxReferenceTraits;

template<typename T, MaxReferenceType refType>
class MaxReference :
	public IMaxReference
{
public:
	MaxReference(T* ref = nullptr) :
		m_ref(ref)
	{}

	MaxReferenceType GetType() const
	{
		return refType;
	}

	ReferenceTarget* GetRef() const
	{
		return m_ref;
	}

	void SetRef(ReferenceTarget* ref)
	{
		m_ref = dynamic_cast<T*>(ref);
		FASSERT(m_ref != nullptr || ref == nullptr);
	}

private:
	T* m_ref;
};


template<
	typename TDerived,
	typename MaxBase,
	typename... BaseTypes>
	class ReferenceKeeper :
	public BaseTypes...
{
	using ClassTraits = MaxReferenceTraits<TDerived>;
	using IdEnum = typename ClassTraits::Enum;

	static constexpr auto LocalRefsCount = static_cast<size_t>(ClassTraits::Enum::__last);

public:
	ReferenceKeeper()
	{
		InitializeReferences(std::make_index_sequence<LocalRefsCount>());
	}

	int NumRefs() override
	{
		return GetBaseRefsCount() + LocalRefsCount;
	}

	void SetReference(int i, RefTargetHandle rtarg) override
	{
		auto baseRefs = GetBaseRefsCount();

		// Check that the index of reference doesn't belong to the base
		if (i < baseRefs)
		{
			MaxBase::SetReference(i, rtarg);
			return;
		}

		m_localRefs[i - baseRefs]->SetRef(rtarg);
	}

	RefTargetHandle GetReference(int i) override
	{
		auto baseRefs = GetBaseRefsCount();

		// Check that the index of reference doesn't belong to the base
		if (i < baseRefs)
		{
			return MaxBase::GetReference(i);
		}

		return m_localRefs[i - baseRefs]->GetRef();
	}

	template<IdEnum id>
	decltype(auto) GetLocalReference() const
	{
		using RefTraits = typename ClassTraits::ReferenceTraits<id>;
		using Type = typename RefTraits::ObjectType;

		return static_cast<Type*>(m_localRefs[static_cast<int>(id)]->GetRef());
	}

	template<IdEnum id>
	void ReplaceLocalReference(ReferenceTarget* ref)
	{
		auto ret = ReplaceReference(static_cast<int>(id) + GetBaseRefsCount(), ref);
		FASSERT(ret == REF_SUCCEED);
	}

private:
	int GetBaseRefsCount() { return MaxBase::NumRefs(); }

	template<size_t... indices>
	void InitializeReferences(std::index_sequence<indices...>)
	{
		(void)std::initializer_list<int>
		{
			(InitializeReference<indices>(), 0)...
		};
	}

	template<size_t index>
	void InitializeReference()
	{
		using ClassTraits = MaxReferenceTraits<TDerived>;
		using Enum = typename ClassTraits::Enum;
		using RefTraits = typename ClassTraits::ReferenceTraits<static_cast<Enum>(index)>;
		using Type = typename RefTraits::ObjectType;

		m_localRefs[index] = std::make_unique<MaxReference<Type, RefTraits::RefType>>();
	}

	std::array<std::unique_ptr<IMaxReference>, LocalRefsCount> m_localRefs;
};

FIRERENDER_NAMESPACE_END
