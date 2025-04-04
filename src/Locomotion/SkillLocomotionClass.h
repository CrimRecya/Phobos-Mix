#pragma once

#include <LocomotionClass.h>
#include <FootClass.h>
#include <VocClass.h>
#include <Utilities/Debug.h>
#include <MapClass.h>

#include <Interfaces.h>

#include <comip.h>
#include <comdef.h>

class __declspec(uuid("4A582751-9839-11d1-B709-00A024DDAFD1"))
	SkillLocomotionClass : public LocomotionClass, public IPiggyback
{
public:
	// IUnknown
	virtual HRESULT __stdcall QueryInterface(REFIID iid, LPVOID* ppvObject) override
	{
		HRESULT hr = this->LocomotionClass::QueryInterface(iid, ppvObject);

		if (hr != E_NOINTERFACE)
			return hr;

		if (iid == __uuidof(IPiggyback))
		{
			*ppvObject = static_cast<IPiggyback*>(this);
			this->AddRef();

			return S_OK;
		}

		*ppvObject = nullptr;

		return E_NOINTERFACE;
	}
	virtual ULONG __stdcall AddRef() override { return this->LocomotionClass::AddRef(); }
	virtual ULONG __stdcall Release() override { return this->LocomotionClass::Release(); }

	// IPersist
	virtual HRESULT __stdcall GetClassID(CLSID* pClassID) override
	{
		if (pClassID == nullptr)
			return E_POINTER;

		*pClassID = __uuidof(this);

		return S_OK;
	}

	// IPersistStream
//	virtual HRESULT __stdcall IsDirty() override { return !this->Dirty; }
	virtual HRESULT __stdcall Load(IStream* pStm) override
	{
		HRESULT hr = this->LocomotionClass::Load(pStm);

		if (FAILED(hr))
			return hr;

		if (this)
		{
			this->Piggybacker.Detach();
			new (this) SkillLocomotionClass(noinit_t());
		}

		bool piggybackerPresent = false;
		hr = pStm->Read(&piggybackerPresent, sizeof(piggybackerPresent), nullptr);

		if (!piggybackerPresent)
			return hr;

		hr = OleLoadFromStream(pStm, __uuidof(ILocomotion), reinterpret_cast<LPVOID*>(&this->Piggybacker));

		return hr;
	}
	virtual HRESULT __stdcall Save(IStream* pStm, BOOL fClearDirty) override
	{
		HRESULT hr = this->LocomotionClass::Save(pStm, fClearDirty);

		if (FAILED(hr))
			return hr;

		bool piggybackerPresent = this->Piggybacker != nullptr;
		hr = pStm->Write(&piggybackerPresent, sizeof(piggybackerPresent), nullptr);

		if (!piggybackerPresent)
			return hr;

		IPersistStreamPtr piggyPersist(this->Piggybacker);
		hr = OleSaveToStream(piggyPersist, pStm);

		return hr;
	}
/*	virtual HRESULT __stdcall GetSizeMax(ULARGE_INTEGER* pcbSize) override
	{
		if (pcbSize == nullptr)
			return E_POINTER;

		return this->LocomotionClass::GetSizeMax(pcbSize);
	}*/
	virtual int Size() override { return sizeof(*this); }

	// ILocomotion
/*	virtual HRESULT __stdcall Link_To_Object(void* pointer) override
	{
		HRESULT hr = this->LocomotionClass::Link_To_Object(pointer);

		if (SUCCEEDED(hr))
			Debug::Log("SkillLocomotionClass - Sucessfully linked to \"%s\"\n", Owner->get_ID());

		return hr;
	}*/
	virtual bool __stdcall Is_Moving() override { JMP_STD(0x4AFB80); }
	virtual CoordStruct __stdcall Destination() override { JMP_STD(0x4AFC90); }
	virtual CoordStruct __stdcall Head_To_Coord() override { JMP_STD(0x4AFCC0); }
	//virtual Move __stdcall Can_Enter_Cell(CellStruct cell) override { return Move::OK; }
	//virtual bool __stdcall Is_To_Have_Shadow() override { return true; }
	virtual Matrix3D __stdcall Draw_Matrix(VoxelIndexKey* key) override { JMP_STD(0x4AFF60); }
	virtual Matrix3D __stdcall Shadow_Matrix(VoxelIndexKey* key) override { JMP_STD(0x4B0410); }
	//virtual Point2D __stdcall Draw_Point() override { return this->LocomotionClass::Draw_Point(); } // Point2D*
	//virtual Point2D __stdcall Shadow_Point() override { return this->LocomotionClass::Shadow_Point(); } // Point2D*
	//virtual VisualType __stdcall Visual_Character(bool raw) override { return VisualType::Normal; }
	virtual int __stdcall Z_Adjust() override { return 0; }
	virtual ZGradient __stdcall Z_Gradient() override { return ZGradient::Deg90; }
	virtual bool __stdcall Process() override { JMP_STD(0x4B0500); }
	virtual void __stdcall Move_To(CoordStruct to) override { JMP_STD(0x4AFD40); }
	virtual void __stdcall Stop_Moving() override { JMP_STD(0x4AFE00); }
	virtual void __stdcall Do_Turn(DirStruct coord) override { JMP_STD(0x4B0EF0); }
	virtual void __stdcall Unlimbo() override { JMP_STD(0x4B04D0); }
	//virtual void __stdcall Tilt_Pitch_AI() override {}
	//virtual bool __stdcall Power_On() override { return this->LocomotionClass::Power_On(); }
	//virtual bool __stdcall Power_Off() override { return this->LocomotionClass::Power_Off(); }
	//virtual bool __stdcall Is_Powered() override { return this->Powered; }
	//virtual bool __stdcall Is_Ion_Sensitive() override { return false; }
	//virtual bool __stdcall Push(DirStruct dir) override { return false; }
	//virtual bool __stdcall Shove(DirStruct dir) override { return false; }
	virtual void __stdcall Force_Track(int track, CoordStruct coord) override { JMP_STD(0x4B0C40); }
	virtual Layer __stdcall In_Which_Layer() override { return Layer::Ground; }
	//virtual void __stdcall Force_Immediate_Destination(CoordStruct coord) override {}
	virtual void __stdcall Force_New_Slope(int ramp) override { JMP_STD(0x4AFB40); }
	virtual bool __stdcall Is_Moving_Now() override { JMP_STD(0x4AFC20); }
	//virtual int __stdcall Apparent_Speed() override { return this->LinkedTo->GetCurrentSpeed(); }
	//virtual int __stdcall Drawing_Code() override { return 0; }
	//virtual FireError __stdcall Can_Fire() override { return FireError::OK; }
	//virtual int __stdcall Get_Status() override { return 0; }
	//virtual void __stdcall Acquire_Hunter_Seeker_Target() override {}
	//virtual bool __stdcall Is_Surfacing() override { return false; }
	virtual void __stdcall Mark_All_Occupation_Bits(MarkType mark) override { JMP_STD(0x4B48D0); }
	virtual bool __stdcall Is_Moving_Here(CoordStruct to) override { JMP_STD(0x4B4920); }
	virtual bool __stdcall Will_Jump_Tracks() override { JMP_STD(0x4B4B00); }
	//virtual bool __stdcall Is_Really_Moving_Now() override { return this->Is_Moving_Now(); }
	//virtual void __stdcall Stop_Movement_Animation() override {}
	//virtual void __stdcall Limbo() override {}
	virtual void __stdcall Lock() override { this->UnLocked = false; }
	virtual void __stdcall Unlock() override { this->UnLocked = true; }
	virtual int __stdcall Get_Track_Number() override { return this->TrackNumber; }
	virtual int __stdcall Get_Track_Index() override { return this->TrackIndex; }
	virtual int __stdcall Get_Speed_Accum() override { return this->SpeedAccum; }

	//IPiggy
	virtual HRESULT __stdcall Begin_Piggyback(ILocomotion* pointer) override
	{
		if (!pointer)
			return E_POINTER;

		if (this->Piggybacker)

			return E_FAIL;

		this->Piggybacker = pointer;

		return S_OK;
	}
	virtual HRESULT __stdcall End_Piggyback(ILocomotion** pointer) override
	{
		if (!pointer)
			return E_POINTER;

		if (!this->Piggybacker)
			return S_FALSE;

		*pointer = this->Piggybacker.Detach();

		return S_OK;
	}
	virtual bool __stdcall Is_Ok_To_End() override
	{
		return this->Piggybacker && !this->LinkedTo->IsAttackedByLocomotor;
	}
	virtual HRESULT __stdcall Piggyback_CLSID(GUID* classid) override
	{
		HRESULT hr;

		if (classid == nullptr)
			return E_POINTER;

		if (this->Piggybacker)
		{
			IPersistStreamPtr piggyAsPersist(this->Piggybacker);
			hr = piggyAsPersist->GetClassID(classid);
		}
		else
		{
			if (reinterpret_cast<IPiggyback*>(this) == nullptr)
				return E_FAIL;

			IPersistStreamPtr thisAsPersist(this);

			if (thisAsPersist == nullptr)
				return E_FAIL;

			hr = thisAsPersist->GetClassID(classid);
		}

		return hr;
	}
	virtual bool __stdcall Is_Piggybacking() override
	{
		return this->Piggybacker != nullptr;
	}

public:
	inline SkillLocomotionClass() : LocomotionClass { }
		, PreviousRamp { 0 }
		, CurrentRamp { 0 }
		, SlopeTimer {}
		, TargetCoord { CoordStruct::Empty }
		, HeadToCoord { CoordStruct::Empty }
		, SpeedAccum { 0 }
		, MovementSpeed { 0.0 }
		, TrackNumber { -1 }
		, TrackIndex { -1 }
		, IsOnShortTrack { false }
		, IsTurretLockedDown { 0 }
		, IsRotating { false }
		, IsDriving { false }
		, IsRocking { false }
		, UnLocked { true }
		, IsForward { true }
		, IsMoving { false }
		, Piggybacker { nullptr }
		, Standby { 0 }
	{ }

	inline SkillLocomotionClass(noinit_t) : LocomotionClass { noinit_t() } { }
	inline virtual ~SkillLocomotionClass() override = default;

public:
	int PreviousRamp;
	int CurrentRamp;
	RateTimer SlopeTimer;
	CoordStruct TargetCoord;
	CoordStruct HeadToCoord;
	int SpeedAccum;
	double MovementSpeed;
	int TrackNumber;
	int TrackIndex;
	bool IsOnShortTrack;
	char IsTurretLockedDown;
	bool IsRotating;
	bool IsDriving;
	bool IsRocking;
	bool UnLocked;
	bool IsForward;
	bool IsMoving;
	ILocomotionPtr Piggybacker;
	int Standby;
};
