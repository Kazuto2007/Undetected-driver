#ifndef ENTRYPOINT_CPP
#define ENTRYPOINT_CPP
#define debug false

#include <ntifs.h>
#include <windef.h>
#include <cstdint>
#include <intrin.h>
#include <ntimage.h>

#include "kernel/xor.h"
#include "kernel/structures.hpp"
#include "impl/imports.h"

#include "impl/communication/interface.h"

#include "impl/scanner.h"
#include "impl/modules.h"

#include "requests/get_module_base.cpp"
#include "requests/read_physical_memory.cpp"
#include "requests/write_physical_memory.cpp"
#include "requests/signature_scanner.cpp"
#include "requests/virtual_allocate.cpp"

#include "impl/invoked.h"

_declspec(noinline) auto manual_mapped_entry(
	PDRIVER_OBJECT driver_obj,
	PUNICODE_STRING registry_path ) -> long
{
	UNREFERENCED_PARAMETER( registry_path );

	// find a better way to execute it, I don't care.
	std::uint8_t execute[ ] = {

		0x48, 0xB8, // -> mov, rax
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // -> address
		0xFF, 0xE0 // -> jmp rax

	};

	*reinterpret_cast< void** >(&execute [ 2 ]) = &vortex::io_dispatch;
	if ( !NT_SUCCESS( modules::write_address( globals::entry_point, execute, sizeof( execute ), true ) ) )
		return driver::status::failed_sanity_check;

	*reinterpret_cast< void** >(&execute [ 2 ]) = &vortex::io_close;
	auto create_close = (globals::cave_base + sizeof( execute ));
	if ( !NT_SUCCESS( modules::write_address( create_close, execute, sizeof( execute ), true ) ) )
		return driver::status::failed_sanity_check;

	UNICODE_STRING device;
	UNICODE_STRING dos_device;

	qtx_import( RtlInitUnicodeString )(&device, _( DEVICE_NAME ));
	qtx_import( RtlInitUnicodeString )(&dos_device, _( DOS_NAME ));

	PDEVICE_OBJECT device_obj = nullptr;
	auto status = qtx_import( IoCreateDevice )(driver_obj,
		0,
		&device,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		false,
		&device_obj);

	if ( NT_SUCCESS( status ) )
	{
		SetFlag( driver_obj->Flags, DO_BUFFERED_IO );

		driver_obj->MajorFunction [ IRP_MJ_CREATE ] = reinterpret_cast< PDRIVER_DISPATCH >(&vortex::io_close);
		driver_obj->MajorFunction [ IRP_MJ_CLOSE ] = reinterpret_cast< PDRIVER_DISPATCH >(&vortex::io_close);
		driver_obj->MajorFunction [ IRP_MJ_DEVICE_CONTROL ] = reinterpret_cast< PDRIVER_DISPATCH >(globals::entry_point);
		driver_obj->DriverUnload = nullptr;

		ClearFlag( device_obj->Flags, DO_DIRECT_IO );
		ClearFlag( device_obj->Flags, DO_DEVICE_INITIALIZING );

		status = qtx_import( IoCreateSymbolicLink )(&dos_device, &device);
		if ( !NT_SUCCESS( status ) ) {
			qtx_import( IoDeleteDevice )(device_obj);
		}

	}

	return status;
}

_declspec(noinline) auto initialize_hook( ) -> driver::status
{	

	globals::ntos_image_base = modules::get_ntos_base_address( );
	if ( !globals::ntos_image_base ) {
		print_dbg( _( " [sanity] -> failed to get ntoskrnl image.\n" ) );
		return driver::status::failed_sanity_check;
	}

	auto io_create_driver_t = reinterpret_cast< void* >( modules::get_kernel_export( globals::ntos_image_base, _( "IoCreateDriver" ) ));
	if ( !io_create_driver_t ) {
		print_dbg( _( " [sanity] -> failed to get IoCreateDriver.\n" ) );
		return driver::status::failed_sanity_check;
	}

	*reinterpret_cast<void** >( &globals::io_create_driver ) = io_create_driver_t;

	/*
	const auto random = modules::get_random( );
	switch ( random % 2 )
	{
	case 0:
		break;
	case 1:
		break;
	}*/

	const auto target_module = modules::get_kernel_module( _( "spaceport.sys" ) );
	if ( !target_module ) {
		print_dbg( _( " [sanity] -> failed to find target module.\n" ) );
		return driver::status::failed_sanity_check;
	}

	BYTE section_char[ ] = { 'I', 'N', 'I', 'T','\0' };
	globals::cave_base = modules::find_section( target_module, reinterpret_cast< char* >(section_char) );
	if ( !globals::cave_base ) {
		print_dbg( _( " [sanity] -> failed to find module section.\n" ) );
		return driver::status::failed_sanity_check;
	}

	// get code cave a different way please
	globals::cave_base = globals::cave_base - 0x30;

	crt::kmemset( &section_char, 0, sizeof( section_char ) );

	// find a better way to execute it, I don't care.
	std::uint8_t execute[ ] = {
		
		0x48, 0xB8, // -> mov, rax
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // -> address
		0xFF, 0xE0 // -> jmp rax

	};

	globals::entry_point = globals::cave_base;

	*reinterpret_cast< void** >( &execute[2] ) = &manual_mapped_entry;
	modules::write_address( 
		globals::entry_point,
		execute, 
		sizeof( execute ), 
		true 
	);

	return driver::status::successful_operation;
}

_declspec(noinline) auto initialize_dkom( ) -> driver::status
{

	const auto mi_process_loader_entry_t = reinterpret_cast<void*>(modules::find_pattern(
		globals::ntos_image_base, 
		_( "\x48\x8B\xC4\x48\x89\x58\x08\x48\x89\x68\x18\x48\x89\x70\x20\x57\x48\x83\xEC\x30\x65\x48\x8B\x2C\x25\x00\x00\x00\x00\x48\x83\xCE\xFF" ),
		_( "xxxxxxxxxxxxxxxxxxxxxxxxx????xxxx" ) 
	));
	if ( !mi_process_loader_entry_t ) {
		print_dbg( _( " [sanity] -> failed to find dkom.\n" ) );
		return driver::status::failed_sanity_check;
	}
	*reinterpret_cast< void** >(&globals::mi_process_loader_entry) = mi_process_loader_entry_t;


	return driver::status::failed_sanity_check;
}

_declspec(noinline) auto initialize_ioctl( ) -> driver::status
{

	const auto result = globals::io_create_driver(
		nullptr,
		reinterpret_cast< DRIVER_INITIALIZE* >(globals::entry_point)
	);
	if ( !NT_SUCCESS( result ) ) {
		return driver::status::failed_sanity_check;
	}
	
	// this code is if you're loading signed driver or dse exploit
	// start dkom
	/*
	const auto ret = globals::mi_process_loader_entry(
		driver_obj->DriverSection,
		0
	);
	if ( !ret ) {
		print_dbg( _(" [log] -> failed to dkom.\n") );
		return driver::status::failed_sanity_check;
	}

	InitializeListHead(  &reinterpret_cast< PLDR_DATA_TABLE_ENTRY>(
			driver_obj->DriverSection)->InLoadOrderModuleList 
	);

	InitializeListHead( &reinterpret_cast< PLDR_DATA_TABLE_ENTRY >(
		driver_obj->DriverSection)->InMemoryOrderModuleList 
	);

	driver_obj->DriverSection = nullptr;
	driver_obj->DriverStart = nullptr;
	driver_obj->DriverSize = 0;
	driver_obj->DriverUnload = nullptr;
	driver_obj->DriverInit = nullptr;
	driver_obj->DeviceObject = nullptr;

	const auto result = manual_mapped_entry( 
		driver_obj, 
		registry_path 
	);
	if ( !NT_SUCCESS( result ) ) {
		return driver::status::failed_sanity_check;;
	}*/

	return driver::status::successful_operation;
}

// this driver can be loaded or manual mapped :) 
//auto driver_entry( PDRIVER_OBJECT driver_obj, PUNICODE_STRING registry_path ) -> NTSTATUS
auto driver_entry( PDRIVER_OBJECT, PUNICODE_STRING ) -> NTSTATUS
{

	if ( initialize_hook( ) != driver::status::successful_operation )
		return driver::status::failed_intialization;

	// if driver is signed or loaded using dse
	//if ( initialize_dkom( ) != driver::status::successful_operation )
	//	return driver::status::failed_intialization;

	if ( initialize_ioctl( ) != driver::status::successful_operation )
		return driver::status::failed_intialization;

	print_dbg( _(" [driver] -> initialized.\n") );

	return STATUS_SUCCESS;
}

#endif // ENTRYPOINT_CPP